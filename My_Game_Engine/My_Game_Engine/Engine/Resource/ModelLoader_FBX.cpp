#include "ModelLoader_FBX.h"
#include "GameEngine.h"
#include "TextureLoader.h"
#include "MetaIO.h"
#include "DXMathUtils.h"
#include "Mesh.h"
#include "Model_Avatar.h"
#include "AnimationClip.h"

ModelLoader_FBX::ModelLoader_FBX()
{
}


bool ModelLoader_FBX::Load(const std::string& path, std::string_view alias, LoadResult& result)
{
    m_meshMap.clear();

    RendererContext ctx = GameEngine::Get().Get_UploadContext();
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    FbxManager* fbxManager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(ios);

    FbxImporter* importer = FbxImporter::Create(fbxManager, "");
    if (!importer->Initialize(path.c_str(), -1, fbxManager->GetIOSettings()))
    {
        OutputDebugStringA(("[FBX SDK] Failed to load: " + path + "\n").c_str());
        importer->Destroy();
        fbxManager->Destroy();
        return false;
    }

    FbxScene* scene = FbxScene::Create(fbxManager, "scene");
    importer->Import(scene);
    importer->Destroy();

    bool hasMeshes = false;
    int srcMeshCount = scene->GetSrcObjectCount<FbxMesh>();

    for (int i = 0; i < srcMeshCount; ++i)
    {
        FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
        if (!mesh) continue;

        if (mesh->GetControlPointsCount() > 0 && mesh->GetPolygonCount() > 0)
        {
            hasMeshes = true;
            break; 
        }
    }

    int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    bool hasAnims = (animStackCount > 0);

    if (!hasMeshes && !hasAnims)
    {
        fbxManager->Destroy();
        return false;
    }

    std::shared_ptr<Model> model = nullptr;
    if (hasMeshes)
    {
        model = std::make_shared<Model>();
        model->SetAlias(std::filesystem::path(path).stem().string());
        model->SetPath(path);
        rs->RegisterResource(model);
        result.modelId = model->GetId();
    }

    std::unordered_map<std::string, int> matNameCount;
    std::unordered_map<FbxSurfaceMaterial*, UINT> matMap;

    if (hasMeshes)
    {
        int matCount = scene->GetMaterialCount();

        if (matCount > 0)
        {
            std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
            std::filesystem::create_directories(matDir);

            for (int i = 0; i < matCount; i++)
            {
                FbxSurfaceMaterial* fbxMat = scene->GetMaterial(i);
                if (!fbxMat) continue;

                std::string baseName = fbxMat->GetName();
                if (baseName.empty()) baseName = "Material_" + std::to_string(i);
                std::string uniqueName = baseName;
                if (matNameCount.find(baseName) != matNameCount.end())
                {
                    int count = ++matNameCount[baseName];
                    uniqueName = baseName + "_" + std::to_string(count);
                }
                else matNameCount[baseName] = 0;

                std::string matFilePath = (matDir / (uniqueName + ".mat")).string();

                auto mat = rs->LoadOrReuse<Material>(matFilePath, uniqueName, ctx,
                    [&]() -> std::shared_ptr<Material> {
                        auto newMat = std::make_shared<Material>();
                        newMat->FromFbxSDK(fbxMat);
                        auto texIds = TextureLoader::LoadFromFbx(ctx, fbxMat, path, newMat);
                        result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
                        return newMat;
                    }
                );

                if (!mat) continue;
                matMap[fbxMat] = mat->GetId();

                if (mat->diffuseTexId != Engine::INVALID_ID)
                    result.textureIds.push_back(mat->diffuseTexId);
                if (mat->normalTexId != Engine::INVALID_ID)
                    result.textureIds.push_back(mat->normalTexId);
                if (mat->roughnessTexId != Engine::INVALID_ID)
                    result.textureIds.push_back(mat->roughnessTexId);
                if (mat->metallicTexId != Engine::INVALID_ID)
                    result.textureIds.push_back(mat->metallicTexId);


                result.materialIds.push_back(mat->GetId());
            }
        }
    }

    std::shared_ptr<Model_Avatar> modelAvatar = nullptr;
    std::shared_ptr<Skeleton> skeletonRes = nullptr;

    if (hasMeshes || hasAnims)
    {
        bool isTemporary = !hasMeshes;

        std::string skelAlias = hasMeshes ? model->GetAlias() : std::filesystem::path(path).stem().string();
        std::string skelPath = path + ".skel";

        skeletonRes = rs->LoadOrReuse<Skeleton>(skelPath, skelAlias + "_Skeleton", ctx,
            [&]() -> std::shared_ptr<Skeleton> {
                auto skel = BuildSkeleton(scene);
                if (skel) skel->SetTemporary(isTemporary);
                return skel;
            }
        );

        if (skeletonRes && skeletonRes->GetBoneCount() > 0)
        {
            if (!isTemporary) result.skeletonId = skeletonRes->GetId();

            std::string avatarPath = path + ".avatar";
            modelAvatar = rs->LoadOrReuse<Model_Avatar>(avatarPath, skelAlias + "_Avatar", ctx,
                [&]() -> std::shared_ptr<Model_Avatar> {
                    auto avatar = std::make_shared<Model_Avatar>();
                    avatar->SetDefinitionType(DefinitionType::Humanoid);
                    avatar->AutoMap(skeletonRes);
                    if (avatar) avatar->SetTemporary(isTemporary);

                    return avatar;
                }
            );

            if (modelAvatar && !isTemporary) 
                result.avatarId = modelAvatar->GetId();

            if (model)
            {
                model->SetSkeleton(skeletonRes);
                if (modelAvatar) 
                    model->SetAvatarID(modelAvatar->GetId());
            }
        }
        else
        {
            skeletonRes = nullptr;
        }
    }

    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    if (hasMeshes && scene->GetRootNode())
        model->SetRoot(ProcessNode(ctx, scene->GetRootNode(), matMap, path, loadedMeshes));

    std::vector<std::shared_ptr<AnimationClip>> loadedClips;
    if (hasAnims && modelAvatar)
    {
        std::string fbxFileName = std::filesystem::path(path).stem().string();
        std::filesystem::path clipDir = std::filesystem::path(path).parent_path() / "AnimationClip" / fbxFileName;
        std::filesystem::create_directories(clipDir);

        for (int i = 0; i < animStackCount; i++)
        {
            FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(i);
            if (!animStack) continue;

            std::string rawClipName = animStack->GetName();
            if (rawClipName.empty()) rawClipName = "Take_" + std::to_string(i + 1);

            std::string uniqueClipName = fbxFileName + "_" + rawClipName;
            std::string clipPath = (clipDir / (uniqueClipName + ".anim")).string();

            auto animationClip = rs->LoadOrReuse<AnimationClip>(clipPath, uniqueClipName, ctx,
                [&]() -> std::shared_ptr<AnimationClip> {
                    return BuildAnimation(scene, animStack, modelAvatar, skeletonRes);
                }
            );

            if (animationClip)
            {
                animationClip->SetAvatar(modelAvatar);
                animationClip->SetSkeleton(skeletonRes);
                loadedClips.push_back(animationClip);
                result.clipIds.push_back(animationClip->GetId());
            }
        }
    }

    if (hasMeshes)
    {
        for (auto& mesh : loadedMeshes)
        {
            if (auto skinned = std::dynamic_pointer_cast<SkinnedMesh>(mesh))
            {
                skinned->Skinning_Skeleton_Bones(skeletonRes);
            }

            result.meshIds.push_back(mesh->GetId());
        }
    }

    FbxMeta meta;
    if (model)
    {
        meta.guid = model->GetGUID();
    }
    else
    {
        meta.guid = rs->GetOrCreateGUID(path);
    }
    meta.path = path;

    for (auto& mesh : loadedMeshes)
    {
        SubResourceMeta s{};
        s.name = mesh->GetAlias();
        s.type = "MESH";
        s.guid = mesh->GetGUID();
        meta.sub_resources.push_back(s);
    }

    for (auto& [fbxMat, matId] : matMap)
    {
        auto mat = rs->GetById<Material>(matId);
        if (!mat) continue;
        SubResourceMeta s{};
        s.name = mat->GetAlias();
        s.type = "MATERIAL";
        s.guid = mat->GetGUID();
        meta.sub_resources.push_back(s);
    }

    for (auto& texId : result.textureIds)
    {
        auto tex = rs->GetById<Texture>(texId);
        if (!tex) continue;
        SubResourceMeta s{};
        s.name = tex->GetAlias();
        s.type = "TEXTURE";
        s.guid = tex->GetGUID();
        meta.sub_resources.push_back(s);
    }

    for (auto& clip : loadedClips)
    {
        SubResourceMeta s{};
        s.name = clip->GetAlias();
        s.type = "ANIMATION_CLIP";
        s.guid = clip->GetGUID();
        meta.sub_resources.push_back(s);
    }

    {
        SubResourceMeta s{};
        s.name = modelAvatar->GetAlias();
        s.type = "MODEL_AVATAR";
        s.guid = modelAvatar->GetGUID();
        meta.sub_resources.push_back(s);
    }

    {
        SubResourceMeta s{};
        s.name = skeletonRes->GetAlias();
        s.type = "SKELETON";
        s.guid = skeletonRes->GetGUID();
        meta.sub_resources.push_back(s);
    }

    MetaIO::SaveFbxMeta(meta);

    if (model)
    { 
        model->SetMeshCount((UINT)result.meshIds.size());
        model->SetMaterialCount((UINT)result.materialIds.size());
        model->SetTextureCount((UINT)result.textureIds.size());
    }

    fbxManager->Destroy();
    return true;
}

std::shared_ptr<Model::Node> ModelLoader_FBX::ProcessNode(
    const RendererContext& ctx,
    FbxNode* fbxNode,
    std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
    const std::string& path,
    std::vector<std::shared_ptr<Mesh>>& loadedMeshes)
{
    if (!fbxNode) return nullptr;

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    auto node = std::make_shared<Model::Node>();
    node->name = fbxNode->GetName();

    FbxAMatrix local = fbxNode->EvaluateLocalTransform();
    XMFLOAT4X4 m;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            m.m[r][c] = static_cast<float>(local.Get(r, c));
    XMStoreFloat4x4(&node->localTransform, XMMatrixTranspose(XMLoadFloat4x4(&m)));

    if (auto mesh = CreateMeshFromNode(fbxNode, matMap, path))
    {
        node->meshes.push_back(mesh);

        bool already_tracked = false;
        for (const auto& existingMesh : loadedMeshes)
        {
            if (existingMesh == mesh)
            {
                already_tracked = true;
                break;
            }
        }
        if (!already_tracked)
            loadedMeshes.push_back(mesh);
    }

    int childCount = fbxNode->GetChildCount();
    for (int i = 0; i < childCount; i++)
    {
        auto child = ProcessNode(ctx, fbxNode->GetChild(i), matMap, path, loadedMeshes);
        if (child) node->children.push_back(child);
    }

    return node;
}

std::shared_ptr<Mesh> ModelLoader_FBX::CreateMeshFromNode(
    FbxNode* fbxNode,
    std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
    const std::string& path)
{
    if (!fbxNode) return nullptr;
    FbxMesh* fbxMesh = fbxNode->GetMesh();
    if (!fbxMesh) return nullptr;


    auto it = m_meshMap.find(fbxMesh);
    if (it != m_meshMap.end())
        return it->second;

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    bool hasSkin = (fbxMesh->GetDeformerCount(FbxDeformer::eSkin) > 0);

    std::shared_ptr<Mesh> mesh = hasSkin
        ? std::make_shared<SkinnedMesh>()
        : std::make_shared<Mesh>();

    mesh->FromFbxSDK(fbxMesh);

    std::string nodePath = fbxNode->GetName();
    mesh->SetAlias(nodePath);
    mesh->SetGUID(MetaIO::CreateGUID(path, nodePath));
    mesh->SetPath(MakeSubresourcePath(path, "mesh", nodePath));

    FbxGeometryElementMaterial* matElem = fbxMesh->GetElementMaterial();
    int polyCount = fbxMesh->GetPolygonCount();

    if (matElem && fbxNode->GetMaterialCount() > 0)
    {
        std::unordered_map<int, std::vector<UINT>> polyByMatSlot;
        int idx = 0;
        for (int p = 0; p < polyCount; p++)
        {
            int matSlot = matElem->GetIndexArray().GetAt(p);
            int vCountInPoly = fbxMesh->GetPolygonSize(p);
            for (int v = 0; v < vCountInPoly; v++)
                polyByMatSlot[matSlot].push_back(mesh->indices[idx++]);
        }

        UINT indexOffset = 0;
        for (auto& [matSlot, idxList] : polyByMatSlot)
        {
            Mesh::Submesh sub{};
            sub.startIndexLocation = indexOffset;
            sub.indexCount = (UINT)idxList.size();
            sub.baseVertexLocation = 0;

            FbxSurfaceMaterial* fbxMat =
                (matSlot >= 0 && matSlot < fbxNode->GetMaterialCount())
                ? fbxNode->GetMaterial(matSlot)
                : nullptr;

            auto it = (fbxMat ? matMap.find(fbxMat) : matMap.end());
            sub.materialId = (it != matMap.end()) ? it->second : Engine::INVALID_ID;

            mesh->submeshes.push_back(sub);
            std::copy(idxList.begin(), idxList.end(), mesh->indices.begin() + indexOffset);
            indexOffset += (UINT)idxList.size();
        }
    }
    else
    {
        Mesh::Submesh sub{};
        sub.indexCount = (UINT)mesh->indices.size();
        sub.startIndexLocation = 0;
        sub.baseVertexLocation = 0;
        sub.materialId = Engine::INVALID_ID;
        mesh->submeshes.push_back(sub);
    }
    mesh->SetAABB();
    rs->RegisterResource(mesh);


    m_meshMap[fbxMesh] = mesh;

    return mesh;
}


std::shared_ptr<Skeleton> ModelLoader_FBX::BuildSkeleton(FbxScene* fbxScene)
{
    auto skeletonRes = std::make_shared<Skeleton>();
    if (!fbxScene) return skeletonRes;

    std::unordered_map<std::string, int> boneNameToIndex;
    std::unordered_map<std::string, FbxAMatrix> boneGlobalBindFbx;
    std::unordered_map<std::string, FbxAMatrix> meshGlobalBindFbx;

    std::function<void(FbxNode*)> AddBoneRecursively = [&](FbxNode* node)
        {
            if (!node) return;
            std::string nodeName = node->GetName();
            if (boneNameToIndex.count(nodeName)) return;

            AddBoneRecursively(node->GetParent());

            int newIndex = static_cast<int>(skeletonRes->mNames.size());
            skeletonRes->mNames.push_back(nodeName);
            boneNameToIndex[nodeName] = newIndex;

            BoneInfo info;
            info.parentIndex = -1;
            XMStoreFloat4x4(&info.bindLocal, XMMatrixIdentity());
            XMStoreFloat4x4(&info.inverseBind, XMMatrixIdentity());
            skeletonRes->mBones.push_back(info);

            FbxTime t0(0);
            boneGlobalBindFbx[nodeName] = node->EvaluateGlobalTransform(t0);
            meshGlobalBindFbx[nodeName].SetIdentity(); 
        };

    int skinCount = fbxScene->GetSrcObjectCount<FbxSkin>();
    for (int s = 0; s < skinCount; ++s)
    {
        FbxSkin* skin = fbxScene->GetSrcObject<FbxSkin>(s);
        if (!skin) continue;

        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; ++c)
        {
            FbxCluster* cluster = skin->GetCluster(c);
            if (!cluster->GetLink()) continue;

            FbxNode* linkNode = cluster->GetLink();
            std::string boneName = linkNode->GetName();

            AddBoneRecursively(linkNode);

            FbxAMatrix linkMatrix, meshMatrix;
            cluster->GetTransformLinkMatrix(linkMatrix);
            cluster->GetTransformMatrix(meshMatrix); 

            boneGlobalBindFbx[boneName] = linkMatrix;
            meshGlobalBindFbx[boneName] = meshMatrix;
        }
    }


    if (skeletonRes->mNames.empty())
    {
        int nodeCount = fbxScene->GetNodeCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            FbxNode* node = fbxScene->GetNode(i);
            if (!node) continue;

            FbxNodeAttribute* attr = node->GetNodeAttribute();
            if (attr && attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
            {
                AddBoneRecursively(node);
            }
        }
    }

    if (skeletonRes->mNames.empty())
    {
        std::function<void(FbxNode*)> AddAllNodes = [&](FbxNode* node) {
            if(!node) return;
            AddBoneRecursively(node);
            for(int k=0; k<node->GetChildCount(); ++k) AddAllNodes(node->GetChild(k));
        };
        AddAllNodes(fbxScene->GetRootNode());

    }

    for (auto& pair : boneNameToIndex)
    {
        int boneIdx = pair.second;
        FbxNode* node = fbxScene->FindNodeByName(pair.first.c_str());
        if (!node) continue;

        FbxNode* parent = node->GetParent();
        if (parent && boneNameToIndex.count(parent->GetName()))
        {
            skeletonRes->mBones[boneIdx].parentIndex = boneNameToIndex[parent->GetName()];
        }
    }

    skeletonRes->SortBoneList();

    FbxAMatrix flip;
    flip.SetIdentity();
    flip[2][2] = -1.0;

    size_t finalBoneCount = skeletonRes->mNames.size();
    for (size_t i = 0; i < finalBoneCount; ++i)
    {
        const std::string& boneName = skeletonRes->mNames[i];

        FbxAMatrix gBone = boneGlobalBindFbx.count(boneName) ? boneGlobalBindFbx[boneName] : FbxAMatrix();
        FbxAMatrix gMesh = meshGlobalBindFbx.count(boneName) ? meshGlobalBindFbx[boneName] : FbxAMatrix();

        if (meshGlobalBindFbx.find(boneName) == meshGlobalBindFbx.end()) gMesh.SetIdentity();

        int parentIdx = skeletonRes->mBones[i].parentIndex;
        FbxAMatrix lBone;

        if (parentIdx < 0)
        {
            lBone = gBone;
        }
        else
        {
            std::string parentName = skeletonRes->mNames[parentIdx];
            FbxAMatrix gParent = boneGlobalBindFbx.count(parentName) ? boneGlobalBindFbx[parentName] : FbxAMatrix();
            lBone = gParent.Inverse() * gBone;
        }

        FbxAMatrix lBoneDX = flip * lBone * flip;

        XMFLOAT4X4 localM;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                localM.m[r][c] = (float)lBoneDX.Get(r, c);
        skeletonRes->mBones[i].bindLocal = localM;

        FbxAMatrix invBind = gBone.Inverse() * gMesh;

        FbxAMatrix invBindDX = flip * invBind * flip;

        XMFLOAT4X4 invBindM;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                invBindM.m[r][c] = (float)invBindDX.Get(r, c);
        skeletonRes->mBones[i].inverseBind = invBindM;
    }

    return skeletonRes;
}

std::shared_ptr<AnimationClip> ModelLoader_FBX::BuildAnimation(
    FbxScene* scene,
    FbxAnimStack* animStack,
    std::shared_ptr<Model_Avatar> modelAvatar,
    std::shared_ptr<Skeleton> skeletonRes)
{
    auto newClip = std::make_shared<AnimationClip>();

    FbxTimeSpan timeSpan = animStack->GetLocalTimeSpan();
    FbxTime::EMode timeMode = scene->GetGlobalSettings().GetTimeMode();
    float ticksPerSecond = (float)FbxTime::GetFrameRate(timeMode);
    float duration = (float)timeSpan.GetDuration().GetSecondDouble();

    newClip->mTicksPerSecond = ticksPerSecond;
    newClip->mDuration = duration;
    newClip->mAvatarDefinitionType = DefinitionType::Humanoid;

    std::map<std::string, std::string> boneNameToKeyMap;
    for (auto const& [key, realBoneName] : modelAvatar->GetBoneMap())
    {
        boneNameToKeyMap[realBoneName] = key;
    }

    FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(0);
    if (!animLayer) return newClip;

    FbxAMatrix flip;
    flip.SetIdentity();
    flip[2][2] = -1.0;

    size_t boneCount = skeletonRes->GetBoneCount();

    newClip->mTracks.reserve(boneCount);

    for (size_t bIdx = 0; bIdx < boneCount; ++bIdx)
    {
        std::string boneName = skeletonRes->GetBoneName(bIdx);

        auto it = boneNameToKeyMap.find(boneName);
        if (it == boneNameToKeyMap.end()) continue;

        std::string abstractKey = it->second;
        AnimationTrack track;

        FbxNode* boneNode = scene->FindNodeByName(boneName.c_str());
        if (!boneNode) continue;

        const BoneInfo& boneInfo = skeletonRes->GetBone((int)bIdx);
        bool isRoot = (boneInfo.parentIndex == -1);

        XMFLOAT3 bindPos = { 0, 0, 0 };
        if (!isRoot)
        {
            bindPos = XMFLOAT3(boneInfo.bindLocal._41, boneInfo.bindLocal._42, boneInfo.bindLocal._43);
        }

        FbxAnimCurve* curveT_X = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
        FbxAnimCurve* curveT_Y = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        FbxAnimCurve* curveT_Z = boneNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

        FbxAnimCurve* curveR_X = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
        FbxAnimCurve* curveR_Y = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        FbxAnimCurve* curveR_Z = boneNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

        FbxAnimCurve* curveS_X = boneNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
        FbxAnimCurve* curveS_Y = boneNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
        FbxAnimCurve* curveS_Z = boneNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

        std::set<float> keyTimes;
        auto CollectKeyTimes = [&](FbxAnimCurve* curve) {
            if (curve) {
                for (int k = 0; k < curve->KeyGetCount(); k++)
                    keyTimes.insert((float)curve->KeyGet(k).GetTime().GetSecondDouble());
            }
            };

        CollectKeyTimes(curveT_X); CollectKeyTimes(curveT_Y); CollectKeyTimes(curveT_Z);
        CollectKeyTimes(curveR_X); CollectKeyTimes(curveR_Y); CollectKeyTimes(curveR_Z);
        CollectKeyTimes(curveS_X); CollectKeyTimes(curveS_Y); CollectKeyTimes(curveS_Z);

        if (keyTimes.empty() && duration > 0.0f)
        {
            keyTimes.insert(0.0f);
        }

        FbxTime fbxTime;
        for (float time : keyTimes)
        {
            fbxTime.SetSecondDouble(time);

            FbxAMatrix local = boneNode->EvaluateLocalTransform(fbxTime);

            FbxAMatrix localDX = flip * local * flip;

            FbxQuaternion q = localDX.GetQ();
            FbxVector4 t = localDX.GetT();
            FbxVector4 s = localDX.GetS();

            XMFLOAT3 finalPos;
            if (isRoot)
            {
                finalPos = XMFLOAT3((float)t[0], (float)t[1], (float)t[2]);
            }
            else
            {
                finalPos = bindPos;
            }

            track.RotationKeys.push_back({ time, XMFLOAT4((float)q[0], (float)q[1], (float)q[2], (float)q[3]) });
            track.PositionKeys.push_back({ time, finalPos });
            track.ScaleKeys.push_back({ time, XMFLOAT3((float)s[0], (float)s[1], (float)s[2]) });
        }

        newClip->mTracks.emplace_back(abstractKey, std::move(track));
    }

    std::sort(newClip->mTracks.begin(), newClip->mTracks.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    return newClip;
}