#include "ModelLoader_Assimp.h"
#include "TextureLoader.h"
#include "GameEngine.h"
#include "MetaIO.h"
#include "Model_Avatar.h"
#include "AnimationClip.h"


ModelLoader_Assimp::ModelLoader_Assimp()
{
}

bool ModelLoader_Assimp::Load(const std::string& path, std::string_view alias, LoadResult& result)
{
    m_loadedMeshes.clear();

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();
    RendererContext ctx = GameEngine::Get().Get_UploadContext();

    Assimp::Importer importer;
    const aiScene* ai_scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_ValidateDataStructure);

    if (!ai_scene)
    {
        OutputDebugStringA(("[Assimp] Failed to load: " + path + "\n").c_str());
        return false;
    }

    bool hasMeshes = ai_scene->HasMeshes();
    bool hasAnims = ai_scene->HasAnimations();

    if (!hasMeshes && !hasAnims)
    {
        OutputDebugStringA(("[Assimp] Empty file: " + path + "\n").c_str());
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
    std::vector<UINT> matIdTable(ai_scene->mNumMaterials);

    if (hasMeshes && ai_scene->mNumMaterials > 0)
    {
        std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
        std::filesystem::create_directories(matDir);

        for (unsigned int i = 0; i < ai_scene->mNumMaterials; i++)
        {
            aiMaterial* aiMat = ai_scene->mMaterials[i];
            aiString aiMatName;
            aiMat->Get(AI_MATKEY_NAME, aiMatName);

            std::string baseName = aiMatName.length > 0 ? aiMatName.C_Str() : "Material_" + std::to_string(i);
            std::string uniqueName = baseName + (matNameCount[baseName]++ > 0 ? "_" + std::to_string(matNameCount[baseName]) : "");
            std::string matFilePath = (matDir / (uniqueName + ".mat")).string();

            auto mat = rs->LoadOrReuse<Material>(matFilePath, uniqueName, ctx,
                [&]() -> std::shared_ptr<Material> {
                    auto newMat = std::make_shared<Material>();
                    newMat->FromAssimp(aiMat);
                    auto texIds = TextureLoader::LoadFromAssimp(ctx, aiMat, path, newMat);
                    result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
                    return newMat;
                }
            );

            if (!mat) continue;
            matIdTable[i] = mat->GetId();
            result.materialIds.push_back(mat->GetId());
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
                return BuildSkeleton(ai_scene);
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

    std::vector<std::shared_ptr<AnimationClip>> loadedClips;
    if (hasAnims && modelAvatar)
    {
        std::filesystem::path clipDir = std::filesystem::path(path).parent_path() / "AnimationClip";
        std::filesystem::create_directories(clipDir);

        for (unsigned int i = 0; i < ai_scene->mNumAnimations; i++)
        {
            aiAnimation* anim = ai_scene->mAnimations[i];
            if (!anim) continue;

            std::string clipName = anim->mName.C_Str();
            if (clipName.empty())
            {
                clipName = "Take_" + std::to_string(i + 1);
            }
            std::string clipPath = (clipDir / (clipName + ".anim")).string();

            auto animationClip = rs->LoadOrReuse<AnimationClip>(clipPath, clipName, ctx,
                [&]() -> std::shared_ptr<AnimationClip>
                {
                    auto newClip = std::make_shared<AnimationClip>();

                    newClip->mTicksPerSecond = (float)anim->mTicksPerSecond > 0 ? (float)anim->mTicksPerSecond : 24.0f;
                    newClip->mDuration = (float)anim->mDuration / newClip->mTicksPerSecond;
                    newClip->mAvatarDefinitionType = DefinitionType::Humanoid;

                    std::map<std::string, std::string> boneNameToKeyMap;
                    for (auto const& [key, realBoneName] : modelAvatar->GetBoneMap())
                    {
                        boneNameToKeyMap[realBoneName] = key;
                    }

                    for (unsigned int c = 0; c < anim->mNumChannels; c++)
                    {
                        aiNodeAnim* channel = anim->mChannels[c];
                        if (!channel || !channel->mNodeName.C_Str()) continue;
                        std::string channelBoneName = channel->mNodeName.C_Str();

                        auto it = boneNameToKeyMap.find(channelBoneName);
                        if (it == boneNameToKeyMap.end()) continue;

                        std::string abstractKey = it->second;
                        AnimationTrack track;

                        for (unsigned int k = 0; k < channel->mNumPositionKeys; k++)
                        {
                            auto key = channel->mPositionKeys[k];
                            track.PositionKeys.push_back({ (float)key.mTime / newClip->mTicksPerSecond, {key.mValue.x, key.mValue.y, key.mValue.z} });
                        }
                        for (unsigned int k = 0; k < channel->mNumRotationKeys; k++)
                        {
                            auto key = channel->mRotationKeys[k];
                            track.RotationKeys.push_back({ (float)key.mTime / newClip->mTicksPerSecond, {key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w} });
                        }
                        for (unsigned int k = 0; k < channel->mNumScalingKeys; k++)
                        {
                            auto key = channel->mScalingKeys[k];
                            track.ScaleKeys.push_back({ (float)key.mTime / newClip->mTicksPerSecond, {key.mValue.x, key.mValue.y, key.mValue.z} });
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

    std::unordered_map<std::string, int> nameCount;

    if (hasMeshes)
    {
        for (unsigned int i = 0; i < ai_scene->mNumMeshes; i++)
        {
            aiMesh* ai_mesh = ai_scene->mMeshes[i];
            const bool isSkinned = ai_mesh->HasBones();

            std::shared_ptr<Mesh> mesh = isSkinned ?
                std::make_shared<SkinnedMesh>() :
                std::make_shared<Mesh>();

            mesh->FromAssimp(ai_mesh);

            std::string baseName = ai_mesh->mName.C_Str();
            if (baseName.empty()) baseName = "Mesh_" + std::to_string(i);
            if (nameCount.count(baseName) > 0)
                baseName += "_" + std::to_string(++nameCount[baseName]);
            else
                nameCount[baseName] = 0;

            mesh->SetAlias(baseName);
            mesh->SetPath(path + "#" + baseName);

            Mesh::Submesh sub{};
            sub.indexCount = (UINT)mesh->indices.size();
            sub.startIndexLocation = 0;
            sub.baseVertexLocation = 0;
            sub.materialId = ai_mesh->mMaterialIndex < matIdTable.size()
                ? matIdTable[ai_mesh->mMaterialIndex]
                : Engine::INVALID_ID;
            mesh->submeshes.push_back(sub);

            if (isSkinned)
            {
                SkinnedMesh* skinned = static_cast<SkinnedMesh*>(mesh.get());
                skinned->Skinning_Skeleton_Bones(model->GetSkeleton());
            }

            rs->RegisterResource(mesh);
            mesh->SetAABB();
            m_loadedMeshes.push_back(mesh);
            result.meshIds.push_back(mesh->GetId());

            if (model)
            {
                model->AddMesh(mesh);
            }
        }

        if (model && ai_scene->mRootNode)
            model->SetRoot(ProcessNode(ai_scene->mRootNode, ai_scene));
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

    for (auto& mesh : m_loadedMeshes)
    {
        SubResourceMeta s{};
        s.name = mesh->GetAlias();
        s.type = "MESH";
        s.guid = mesh->GetGUID();
        meta.sub_resources.push_back(s);
    }

    for (auto& matId : result.materialIds)
    {
        std::shared_ptr<Material> mat = rs->GetById<Material>(matId);
        if (!mat) continue;
        SubResourceMeta s{};
        s.name = mat->GetAlias();
        s.type = "MATERIAL";
        s.guid = mat->GetGUID();
        meta.sub_resources.push_back(s);
    }

    for (auto& texId : result.textureIds)
    {
        std::shared_ptr<Texture> tex = rs->GetById<Texture>(texId);
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
    OutputDebugStringA(("[Assimp] Model loaded successfully: " + path + "\n").c_str());
    return true;
}

std::shared_ptr<Model::Node> ModelLoader_Assimp::ProcessNode(aiNode* ainode, const aiScene* scene)
{
    auto node = std::make_shared<Model::Node>();
    node->name = ainode->mName.C_Str();
    aiMatrix4x4 aim = ainode->mTransformation;
    XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&aim)));
    XMStoreFloat4x4(&node->localTransform, xm);

    for (unsigned int i = 0; i < ainode->mNumMeshes; i++)
        node->meshes.push_back(m_loadedMeshes[ainode->mMeshes[i]]);

    for (unsigned int i = 0; i < ainode->mNumChildren; i++)
        node->children.push_back(ProcessNode(ainode->mChildren[i], scene));

    return node;
}

void FindBoneParent(
    aiNode* node,
    const std::unordered_map<std::string, int>& boneMap,
    std::vector<Bone>& boneList)
{
    std::string nodeName = node->mName.C_Str();

    auto it = boneMap.find(nodeName);
    if (it != boneMap.end())
    {
        aiNode* parentNode = node->mParent;
        while (parentNode)
        {
            std::string parentName = parentNode->mName.C_Str();
            auto parentIt = boneMap.find(parentName);
            if (parentIt != boneMap.end())
            {
                boneList[it->second].parentIndex = parentIt->second;
                break;
            }
            parentNode = parentNode->mParent;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        FindBoneParent(node->mChildren[i], boneMap, boneList);
    }
}

std::shared_ptr<Skeleton> ModelLoader_Assimp::BuildSkeleton(const aiScene* scene)
{
    auto skeletonRes = std::make_shared<Skeleton>();

    //std::unordered_map<std::string, int> boneMap;

    //for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    //{
    //    aiMesh* mesh = scene->mMeshes[m];
    //    for (unsigned int b = 0; b < mesh->mNumBones; b++)
    //    {
    //        aiBone* bone = mesh->mBones[b];
    //        std::string name = bone->mName.C_Str();
    //        if (boneMap.count(name)) continue;

    //        Bone bdata{};
    //        bdata.name = name;
    //        bdata.parentIndex = -1;
    //        XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&bone->mOffsetMatrix)));
    //        XMStoreFloat4x4(&bdata.inverseBind, xm);

    //        boneMap[name] = (int)skeletonRes->BoneList.size();
    //        skeletonRes->BoneList.push_back(bdata);
    //    }
    //}

    //for (unsigned int a = 0; a < scene->mNumAnimations; a++)
    //{
    //    aiAnimation* anim = scene->mAnimations[a];
    //    for (unsigned int c = 0; c < anim->mNumChannels; c++)
    //    {
    //        aiNodeAnim* channel = anim->mChannels[c];
    //        std::string name = channel->mNodeName.C_Str();
    //        if (boneMap.count(name)) continue;

    //        Bone bdata{};
    //        bdata.name = name;
    //        bdata.parentIndex = -1;
    //        XMStoreFloat4x4(&bdata.inverseBind, XMMatrixIdentity());

    //        boneMap[name] = (int)skeletonRes->BoneList.size();
    //        skeletonRes->BoneList.push_back(bdata);
    //    }
    //}

    //if (scene->mRootNode)
    //{
    //    FindBoneParent(scene->mRootNode, boneMap, skeletonRes->BoneList);
    //}

    //skeletonRes->SortBoneList();

    //skeletonRes->BuildNameToIndex();

    return skeletonRes;
}