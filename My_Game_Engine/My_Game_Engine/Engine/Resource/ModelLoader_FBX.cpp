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

    bool hasMeshes = (scene->GetSrcObjectCount<FbxMesh>() > 0);
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
                result.materialIds.push_back(mat->GetId());
            }
        }
    }

    std::shared_ptr<Model_Avatar> modelAvatar = nullptr;
    std::shared_ptr<Skeleton> skeletonRes = nullptr;

    if (hasAnims || hasMeshes)
    {
        std::string skelAlias = hasMeshes ? model->GetAlias() : std::filesystem::path(path).stem().string();
        std::string skelPath = path + ".skel";
        skeletonRes = rs->LoadOrReuse<Skeleton>(skelPath, skelAlias + "_Skeleton", ctx,
            [&]() -> std::shared_ptr<Skeleton> {
                return BuildSkeleton(scene);
            }
        );
        result.skeletonId = skeletonRes->GetId();

        std::string avatarPath = path + ".avatar";
        modelAvatar = rs->LoadOrReuse<Model_Avatar>(avatarPath, skelAlias + "_Avatar", ctx,
            [&]() -> std::shared_ptr<Model_Avatar> {
                auto avatar = std::make_shared<Model_Avatar>();
                avatar->SetDefinitionType(DefinitionType::Humanoid);
                avatar->AutoMap(skeletonRes);
                return avatar;
            }
        );
        result.avatarId = modelAvatar->GetId();

        if (model)
        {
            model->SetSkeleton(skeletonRes);
            model->SetAvatarID(modelAvatar->GetId());
        }
    }

    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    if (hasMeshes && scene->GetRootNode())
        model->SetRoot(ProcessNode(ctx, scene->GetRootNode(), matMap, path, loadedMeshes));

    std::vector<std::shared_ptr<AnimationClip>> loadedClips;
    if (hasAnims && modelAvatar)
    {
        std::filesystem::path clipDir = std::filesystem::path(path).parent_path() / "AnimationClip";
        std::filesystem::create_directories(clipDir);

        for (int i = 0; i < animStackCount; i++)
        {
            FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(i);
            if (!animStack) continue;

            std::string clipName = animStack->GetName();
            if (clipName.empty())
            {
                clipName = "Take_" + std::to_string(i + 1);
            }
            std::string clipPath = (clipDir / (clipName + ".anim")).string();

            auto animationClip = rs->LoadOrReuse<AnimationClip>(clipPath, clipName, ctx,
                [&]() -> std::shared_ptr<AnimationClip>
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

                    for (const auto& bone : skeletonRes->GetBoneList())
                    {
                        auto it = boneNameToKeyMap.find(bone.name);
                        if (it == boneNameToKeyMap.end()) continue;

                        std::string abstractKey = it->second;
                        AnimationTrack track;

                        FbxNode* boneNode = scene->FindNodeByName(bone.name.c_str());
                        if (!boneNode) continue;

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

                            XMFLOAT3 pos = {
                                curveT_X ? curveT_X->Evaluate(fbxTime) : 0.0f,
                                curveT_Y ? curveT_Y->Evaluate(fbxTime) : 0.0f,
                                curveT_Z ? curveT_Z->Evaluate(fbxTime) : 0.0f
                            };
                            XMFLOAT3 rotEuler = {
                                curveR_X ? curveR_X->Evaluate(fbxTime) : 0.0f,
                                curveR_Y ? curveR_Y->Evaluate(fbxTime) : 0.0f,
                                curveR_Z ? curveR_Z->Evaluate(fbxTime) : 0.0f
                            };
                            XMFLOAT3 scl = {
                                curveS_X ? curveS_X->Evaluate(fbxTime) : 1.0f,
                                curveS_Y ? curveS_Y->Evaluate(fbxTime) : 1.0f,
                                curveS_Z ? curveS_Z->Evaluate(fbxTime) : 1.0f
                            };

                            XMFLOAT4 rotQuat = Matrix4x4::QuaternionFromEuler(rotEuler.x, rotEuler.y, rotEuler.z);

                            track.PositionKeys.push_back({ time, pos });
                            track.RotationKeys.push_back({ time, rotQuat });
                            track.ScaleKeys.push_back({ time, scl });
                        }

                        newClip->mTracks[abstractKey] = std::move(track);
                    }
                    return newClip;
                }
            );

            if (animationClip)
            {
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

    MetaIO::SaveFbxMeta(meta);
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
    using namespace DirectX;

    auto skeletonRes = std::make_shared<Skeleton>();
    if (!fbxScene)
        return skeletonRes;

    std::unordered_map<std::string, int> boneNameToIndex;
    boneNameToIndex.reserve(128);

    std::unordered_map<std::string, FbxAMatrix> boneGlobalBindFbx;
    boneGlobalBindFbx.reserve(128);

    int skinCount = fbxScene->GetSrcObjectCount<FbxSkin>();
    for (int s = 0; s < skinCount; ++s)
    {
        FbxSkin* skin = fbxScene->GetSrcObject<FbxSkin>(s);
        if (!skin) continue;

        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; ++c)
        {
            FbxCluster* cluster = skin->GetCluster(c);
            if (!cluster) continue;

            FbxNode* linkNode = cluster->GetLink();
            if (!linkNode) continue;

            std::string boneName = linkNode->GetName();
            if (boneNameToIndex.count(boneName))
            {
                FbxAMatrix linkMatrix;
                cluster->GetTransformLinkMatrix(linkMatrix);
                boneGlobalBindFbx[boneName] = linkMatrix;
                continue;
            }

            Bone bone{};
            bone.name = boneName;
            bone.parentIndex = -1;
            XMStoreFloat4x4(&bone.bindLocal, XMMatrixIdentity());

            int newIndex = static_cast<int>(skeletonRes->BoneList.size());
            boneNameToIndex[boneName] = newIndex;
            skeletonRes->BoneList.push_back(bone);

            FbxAMatrix linkMatrix;
            cluster->GetTransformLinkMatrix(linkMatrix);
            boneGlobalBindFbx[boneName] = linkMatrix;
        }
    }

    int animStackCount = fbxScene->GetSrcObjectCount<FbxAnimStack>();
    if (skinCount == 0 && animStackCount > 0)
    {
        FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(0);
        FbxAnimLayer* animLayer = animStack ? animStack->GetMember<FbxAnimLayer>(0) : nullptr;

        if (animLayer)
        {
            int nodeCount = fbxScene->GetNodeCount();
            for (int n = 0; n < nodeCount; ++n)
            {
                FbxNode* node = fbxScene->GetNode(n);
                if (!node) continue;

                bool hasCurve =
                    node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X) ||
                    node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y) ||
                    node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z) ||
                    node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X) ||
                    node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y) ||
                    node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z) ||
                    node->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X) ||
                    node->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y) ||
                    node->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

                if (!hasCurve)
                    continue;

                std::string boneName = node->GetName();
                if (boneNameToIndex.count(boneName))
                    continue;

                Bone bone{};
                bone.name = boneName;
                bone.parentIndex = -1;
                XMStoreFloat4x4(&bone.bindLocal, XMMatrixIdentity());

                int newIndex = static_cast<int>(skeletonRes->BoneList.size());
                boneNameToIndex[boneName] = newIndex;
                skeletonRes->BoneList.push_back(bone);

                FbxTime t0;
                t0.SetSecondDouble(0.0);
                FbxAMatrix globalAt0 = node->EvaluateGlobalTransform(t0);
                boneGlobalBindFbx[boneName] = globalAt0;
            }
        }
    }

    for (auto& pair : boneNameToIndex)
    {
        const std::string& boneName = pair.first;
        int                boneIdx = pair.second;

        FbxNode* boneNode = fbxScene->FindNodeByName(boneName.c_str());
        if (!boneNode)
            continue;

        FbxNode* parentNode = boneNode->GetParent();
        if (!parentNode)
            continue;

        std::string parentName = parentNode->GetName();
        auto itParent = boneNameToIndex.find(parentName);
        if (itParent != boneNameToIndex.end())
        {
            skeletonRes->BoneList[boneIdx].parentIndex = itParent->second;
        }
        else
        {
            skeletonRes->BoneList[boneIdx].parentIndex = -1;
        }
    }

    skeletonRes->SortBoneList();
    skeletonRes->BuildNameToIndex();

    const size_t boneCount = skeletonRes->BoneList.size();
    skeletonRes->mBindLocal.resize(boneCount);
    skeletonRes->mBindGlobal.resize(boneCount);
    skeletonRes->mInverseBind.resize(boneCount);

    std::vector<FbxAMatrix> globalBindFbx(boneCount);
    for (size_t i = 0; i < boneCount; ++i)
    {
        const std::string& boneName = skeletonRes->BoneList[i].name;

        auto it = boneGlobalBindFbx.find(boneName);
        if (it != boneGlobalBindFbx.end())
        {
            globalBindFbx[i] = it->second;
        }
        else
        {
            FbxNode* node = fbxScene->FindNodeByName(boneName.c_str());
            if (node)
            {
                FbxTime t0;
                t0.SetSecondDouble(0.0);
                globalBindFbx[i] = node->EvaluateGlobalTransform(t0);
            }
            else
            {
                globalBindFbx[i].SetIdentity();
            }
        }
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        int parentIdx = skeletonRes->BoneList[i].parentIndex;

        FbxAMatrix g = globalBindFbx[i];
        FbxAMatrix l;

        if (parentIdx < 0)
        {
            l = g;
        }
        else
        {
            FbxAMatrix parentG = globalBindFbx[parentIdx];
            FbxAMatrix parentInvG = parentG.Inverse();
            l = parentInvG * g;
        }

        XMFLOAT4X4 localM;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                localM.m[r][c] = static_cast<float>(l.Get(r, c));

        skeletonRes->BoneList[i].bindLocal = localM;
        skeletonRes->mBindLocal[i] = localM;

        XMFLOAT4X4 globalM;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                globalM.m[r][c] = static_cast<float>(g.Get(r, c));

        skeletonRes->mBindGlobal[i] = globalM;

        XMMATRIX gDX = XMLoadFloat4x4(&globalM);
        XMMATRIX invDX = XMMatrixInverse(nullptr, gDX);

        XMStoreFloat4x4(&skeletonRes->mInverseBind[i], invDX);
    }

    return skeletonRes;
}
