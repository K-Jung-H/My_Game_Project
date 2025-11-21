#include "ModelLoader_Assimp.h"
#include "TextureLoader.h"
#include "GameEngine.h"
#include "MetaIO.h"
#include "Model_Avatar.h"
#include "AnimationClip.h"

using namespace DirectX;

namespace
{
    XMFLOAT4X4 Mat4FromAssimp(const aiMatrix4x4& mat)
    {
        return XMFLOAT4X4(
            mat.a1, mat.a2, mat.a3, mat.a4,
            mat.b1, mat.b2, mat.b3, mat.b4,
            mat.c1, mat.c2, mat.c3, mat.c4,
            mat.d1, mat.d2, mat.d3, mat.d4
        );
    }

    XMFLOAT3 Vec3FromAssimp(const aiVector3D& v) { return XMFLOAT3(v.x, v.y, v.z); }
    XMFLOAT4 QuatFromAssimp(const aiQuaternion& q) { return XMFLOAT4(q.x, q.y, q.z, q.w); }
}

ModelLoader_Assimp::ModelLoader_Assimp()
{
}

bool ModelLoader_Assimp::Load(const std::string& path, std::string_view alias, LoadResult& result)
{
    m_meshMap.clear();

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();
    RendererContext ctx = GameEngine::Get().Get_UploadContext();

    Assimp::Importer importer;

    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_ValidateDataStructure |
        aiProcess_MakeLeftHanded |
        aiProcess_FlipWindingOrder
    );

    if (!scene)
    {
        OutputDebugStringA(("[Assimp] Failed to load: " + path + "\n").c_str());
        return false;
    }

    bool hasMeshes = scene->HasMeshes();
    bool hasAnims = scene->HasAnimations();

    if (!hasMeshes && !hasAnims) return false;

    std::shared_ptr<Model> model = nullptr;
    if (hasMeshes)
    {
        model = std::make_shared<Model>();
        model->SetAlias(std::filesystem::path(path).stem().string());
        model->SetPath(path);
        rs->RegisterResource(model);
        result.modelId = model->GetId();
    }

    std::vector<UINT> materialIDs;
    if (hasMeshes && scene->HasMaterials())
    {
        std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
        std::filesystem::create_directories(matDir);
        std::unordered_map<std::string, int> matNameCount;

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial* aiMat = scene->mMaterials[i];

            aiString name;
            aiMat->Get(AI_MATKEY_NAME, name);
            std::string baseName = name.C_Str();
            if (baseName.empty()) baseName = "Material_" + std::to_string(i);

            std::string uniqueName = baseName;
            if (matNameCount[baseName]++ > 0) uniqueName += "_" + std::to_string(matNameCount[baseName]);

            std::string matPath = (matDir / (uniqueName + ".mat")).string();

            auto mat = rs->LoadOrReuse<Material>(matPath, uniqueName, ctx, [&]() {
                auto newMat = std::make_shared<Material>();
                auto texIds = TextureLoader::LoadFromAssimp(ctx, aiMat, path, newMat);
                result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
                return newMat;
                });

            if (mat)
            {
                materialIDs.push_back(mat->GetId());
                result.materialIds.push_back(mat->GetId());
            }
            else
            {
                materialIDs.push_back(Engine::INVALID_ID);
            }
        }
    }

    std::shared_ptr<Skeleton> skeletonRes = nullptr;
    std::shared_ptr<Model_Avatar> modelAvatar = nullptr;

    if (hasMeshes || hasAnims)
    {
        std::string skelAlias = hasMeshes ? model->GetAlias() : std::filesystem::path(path).stem().string();
        std::string skelPath = path + ".skel";

        skeletonRes = rs->LoadOrReuse<Skeleton>(skelPath, skelAlias + "_Skeleton", ctx,
            [&]() -> std::shared_ptr<Skeleton> {
                return BuildSkeleton(scene);
            }
        );

        if (skeletonRes && skeletonRes->GetBoneCount() > 0)
        {
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
        else
        {
            skeletonRes = nullptr;
        }
    }

    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    if (hasMeshes && scene->mRootNode)
    {
        auto rootNode = ProcessNode(ctx, scene->mRootNode, scene, path, materialIDs, loadedMeshes);
        if (model)
        {
            model->SetRoot(rootNode);
            for (const auto& mesh : loadedMeshes)
            {
                model->AddMesh(mesh);
            }
        }
    }

    std::vector<std::shared_ptr<AnimationClip>> loadedClips;
    if (hasAnims && modelAvatar && skeletonRes)
    {
        std::filesystem::path clipDir = std::filesystem::path(path).parent_path() / "AnimationClip";
        std::filesystem::create_directories(clipDir);

        for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
        {
            const aiAnimation* anim = scene->mAnimations[i];
            std::string clipName = anim->mName.C_Str();
            if (clipName.empty()) clipName = "Take_" + std::to_string(i + 1);

            std::string clipPath = (clipDir / (clipName + ".anim")).string();

            auto animationClip = rs->LoadOrReuse<AnimationClip>(clipPath, clipName, ctx,
                [&]() -> std::shared_ptr<AnimationClip> {
                    return ProcessAnimation(anim, modelAvatar, skeletonRes);
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

    if (skeletonRes && !loadedMeshes.empty())
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
    if (model) meta.guid = model->GetGUID();
    else meta.guid = rs->GetOrCreateGUID(path);
    meta.path = path;

    for (auto& mesh : loadedMeshes)
    {
        SubResourceMeta s{};
        s.name = mesh->GetAlias();
        s.type = "MESH";
        s.guid = mesh->GetGUID();
        meta.sub_resources.push_back(s);
    }
    for (UINT matId : materialIDs)
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

    return true;
}

std::shared_ptr<Model::Node> ModelLoader_Assimp::ProcessNode(
    const RendererContext& ctx,
    aiNode* node,
    const aiScene* scene,
    const std::string& path,
    const std::vector<UINT>& materialIDs,
    std::vector<std::shared_ptr<Mesh>>& outLoadedMeshes)
{
    auto modelNode = std::make_shared<Model::Node>();
    modelNode->name = node->mName.C_Str();

    XMFLOAT4X4 assimpMat = Mat4FromAssimp(node->mTransformation);
    XMStoreFloat4x4(&modelNode->localTransform, XMMatrixTranspose(XMLoadFloat4x4(&assimpMat)));

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];

        auto mesh = CreateMeshFromNode(meshIndex, scene, path, materialIDs);

        if (mesh)
        {
            modelNode->meshes.push_back(mesh);

            bool exists = false;
            for (auto& m : outLoadedMeshes) if (m == mesh) { exists = true; break; }
            if (!exists) outLoadedMeshes.push_back(mesh);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        auto childNode = ProcessNode(ctx, node->mChildren[i], scene, path, materialIDs, outLoadedMeshes);
        if (childNode)
        {
            modelNode->children.push_back(childNode);
        }
    }

    return modelNode;
}

std::shared_ptr<Mesh> ModelLoader_Assimp::CreateMeshFromNode(
    unsigned int meshIndex,
    const aiScene* scene,
    const std::string& path,
    const std::vector<UINT>& materialIDs)
{
    if (m_meshMap.count(meshIndex))
    {
        return m_meshMap[meshIndex];
    }

    aiMesh* mesh = scene->mMeshes[meshIndex];
    bool hasSkin = mesh->HasBones();
    std::shared_ptr<Mesh> newMesh = hasSkin ? std::make_shared<SkinnedMesh>() : std::make_shared<Mesh>();

    newMesh->FromAssimp(mesh);

    if (newMesh->GetIndexCount() > 0)
    {
        Mesh::Submesh sub{};
        sub.indexCount = newMesh->GetIndexCount();
        sub.startIndexLocation = 0;
        sub.baseVertexLocation = 0;

        if (mesh->mMaterialIndex < materialIDs.size())
        {
            sub.materialId = materialIDs[mesh->mMaterialIndex];
        }
        else
        {
            sub.materialId = Engine::INVALID_ID;
        }
        newMesh->submeshes.push_back(sub);
    }
    else
    {
        return nullptr;
    }

    newMesh->SetAABB();
    newMesh->SetAlias(mesh->mName.C_Str());
    newMesh->SetGUID(MetaIO::CreateGUID(path, mesh->mName.C_Str()));

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();
    rs->RegisterResource(newMesh);

    m_meshMap[meshIndex] = newMesh;

    return newMesh;
}

std::shared_ptr<Skeleton> ModelLoader_Assimp::BuildSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return nullptr;

    std::unordered_set<std::string> boneNames;

    if (scene->HasMeshes())
    {
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[i];
            for (unsigned int b = 0; b < mesh->mNumBones; ++b)
                boneNames.insert(mesh->mBones[b]->mName.C_Str());
        }
    }
    if (scene->HasAnimations())
    {
        for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
        {
            const aiAnimation* anim = scene->mAnimations[i];
            for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                boneNames.insert(anim->mChannels[c]->mNodeName.C_Str());
        }
    }

    if (boneNames.empty()) return nullptr;

    auto skeleton = std::make_shared<Skeleton>();

    std::unordered_map<const aiNode*, bool> keepNode;
    std::function<bool(const aiNode*)> checkNode = [&](const aiNode* node) {
        bool keep = (boneNames.count(node->mName.C_Str()) > 0);
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            if (checkNode(node->mChildren[i])) keep = true;
        keepNode[node] = keep;
        return keep;
        };
    checkNode(scene->mRootNode);

    std::unordered_map<const aiNode*, int> nodeToIndex;
    std::vector<const aiNode*> linearNodes;

    std::function<void(const aiNode*)> collectNodes = [&](const aiNode* node) {
        if (keepNode[node])
        {
            int idx = (int)skeleton->mNames.size();

            skeleton->mNames.push_back(node->mName.C_Str());
            nodeToIndex[node] = idx;
            linearNodes.push_back(node);

            BoneInfo info;
            info.parentIndex = -1;

            XMFLOAT4X4 localAssimp = Mat4FromAssimp(node->mTransformation);
            XMStoreFloat4x4(&info.bindLocal, XMMatrixTranspose(XMLoadFloat4x4(&localAssimp)));

            XMStoreFloat4x4(&info.inverseBind, XMMatrixIdentity());

            skeleton->mBones.push_back(info);
        }
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            collectNodes(node->mChildren[i]);
        };
    collectNodes(scene->mRootNode);

    for (size_t i = 0; i < linearNodes.size(); ++i)
    {
        const aiNode* node = linearNodes[i];
        if (node->mParent && nodeToIndex.count(node->mParent))
        {
            skeleton->mBones[i].parentIndex = nodeToIndex[node->mParent];
        }
    }

    if (scene->HasMeshes())
    {
        std::unordered_map<std::string, aiMatrix4x4> offsetMap;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[i];
            for (unsigned int b = 0; b < mesh->mNumBones; ++b)
            {
                offsetMap[mesh->mBones[b]->mName.C_Str()] = mesh->mBones[b]->mOffsetMatrix;
            }
        }

        for (size_t i = 0; i < skeleton->mNames.size(); ++i)
        {
            const std::string& name = skeleton->mNames[i];
            if (offsetMap.count(name))
            {
                XMFLOAT4X4 offMat = Mat4FromAssimp(offsetMap[name]);
                XMStoreFloat4x4(&skeleton->mBones[i].inverseBind, XMMatrixTranspose(XMLoadFloat4x4(&offMat)));
            }
        }
    }

    skeleton->SortBoneList();
    return skeleton;
}

std::shared_ptr<AnimationClip> ModelLoader_Assimp::ProcessAnimation(const aiAnimation* anim,
    std::shared_ptr<Model_Avatar> avatar, std::shared_ptr<Skeleton> skeleton)
{
    auto clip = std::make_shared<AnimationClip>();
    clip->mAvatarDefinitionType = avatar->GetDefinitionType();
    clip->mTicksPerSecond = (anim->mTicksPerSecond != 0.0) ? (float)anim->mTicksPerSecond : 24.0f;
    clip->mDuration = (float)(anim->mDuration / clip->mTicksPerSecond);

    std::map<std::string, std::string> boneMap;
    for (auto const& [k, v] : avatar->GetBoneMap()) boneMap[v] = k;

    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        aiNodeAnim* channel = anim->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();
        if (boneMap.find(boneName) == boneMap.end()) continue;

        std::string key = boneMap[boneName];
        AnimationTrack track;

        auto ToTime = [&](double t) { return (float)(t / clip->mTicksPerSecond); };

        for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k)
            track.PositionKeys.push_back({ ToTime(channel->mPositionKeys[k].mTime), Vec3FromAssimp(channel->mPositionKeys[k].mValue) });

        for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k)
            track.RotationKeys.push_back({ ToTime(channel->mRotationKeys[k].mTime), QuatFromAssimp(channel->mRotationKeys[k].mValue) });

        for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k)
            track.ScaleKeys.push_back({ ToTime(channel->mScalingKeys[k].mTime), Vec3FromAssimp(channel->mScalingKeys[k].mValue) });

        clip->mTracks[key] = std::move(track);
    }
    return clip;
}