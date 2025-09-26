#include "pch.h"
#include "ResourceRegistry.h"
#include "Model.h"

LoadResult ResourceRegistry::Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx)
{
    LoadResult result;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals);

    if (!scene) 
    {
        std::string errMsg = "[Assimp] Failed to load: " + path + "\nReason: " + importer.GetErrorString() + "\n";
        OutputDebugStringA(errMsg.c_str());
        return result;
    }

    auto model = std::make_shared<Model>();
    model->SetId(mNextResourceID++);
    model->SetPath(path);
    model->SetAlias(std::string(alias));
    manager.Add(model);
    result.modelId = model->GetId();

    std::vector<UINT> matIdTable(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) 
    {
        auto mat = std::make_shared<Material>();
        mat->FromAssimp(scene->mMaterials[i]);
        mat->SetId(mNextResourceID++);
        mat->SetPath(path);
        mat->SetAlias(std::string(alias) + "_mat" + std::to_string(i));

        manager.Add(mat);
        result.materialIds.push_back(mat->GetId());
        matIdTable[i] = mat->GetId();

        auto texIds = LoadMaterialTextures(manager, scene->mMaterials[i], path, mat, ctx);
        result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
    }

    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) 
    {
        auto mesh = std::make_shared<Mesh>();
        mesh->FromAssimp(scene->mMeshes[i]);
        mesh->SetId(mNextResourceID++);
        mesh->SetPath(path);
        mesh->SetAlias(std::string(alias) + "_mesh" + std::to_string(i));

        Mesh::Submesh sub{};
        sub.indexCount = static_cast<UINT>(mesh->indices.size());
        sub.startIndexLocation = 0;
        sub.baseVertexLocation = 0;
        UINT matIndex = scene->mMeshes[i]->mMaterialIndex;
        sub.materialId = (matIndex < matIdTable.size()) ? matIdTable[matIndex] : Engine::INVALID_ID;
        mesh->submeshes.push_back(sub);

        manager.Add(mesh);
        result.meshIds.push_back(mesh->GetId());
        loadedMeshes.push_back(mesh);

        model->AddMesh(mesh);
    }

    if (scene->mRootNode)
        model->SetRoot(ProcessNode(scene->mRootNode, scene, loadedMeshes));

    // ---------- Skeleton ----------
    Skeleton skeleton = BuildSkeleton(scene);
    model->SetSkeleton(skeleton);

    return result;
}



std::vector<UINT> ResourceRegistry::LoadMaterialTextures(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx)
{
    std::vector<UINT> textureIds;

    static const std::vector<std::pair<aiTextureType, std::string>> textureTypes = 
    {
        { aiTextureType_DIFFUSE,           "diffuse" },
        { aiTextureType_NORMALS,           "normal" },
        { aiTextureType_DIFFUSE_ROUGHNESS, "roughness" },
        { aiTextureType_METALNESS,         "metallic" }
    };

    for (auto& [type, suffix] : textureTypes)
    {
        if (material->GetTextureCount(type) == 0)
            continue;

        aiString texPath;
        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
        {
            std::filesystem::path baseDir = std::filesystem::path(basePath).parent_path();
            std::filesystem::path texRel = std::filesystem::path(texPath.C_Str());
            std::filesystem::path fullPath = baseDir / texRel;

            if (auto tex = manager.GetByPath<Texture>(fullPath.string()))
            {
                textureIds.push_back(tex->GetId());

                switch (type)
                {
                case aiTextureType_DIFFUSE:
                    mat->diffuseTexId = tex->GetId();
                    mat->diffuseTexSlot = tex->GetSlot();
                    break;
                case aiTextureType_NORMALS:
                    mat->normalTexId = tex->GetId();
                    mat->normalTexSlot = tex->GetSlot();
                    break;
                case aiTextureType_DIFFUSE_ROUGHNESS:
                    mat->roughnessTexId = tex->GetId();
                    mat->roughnessTexSlot = tex->GetSlot();
                    break;
                case aiTextureType_METALNESS:
                    mat->metallicTexId = tex->GetId();
                    mat->metallicTexSlot = tex->GetSlot();
                    break;
                }

                continue;
            }

            auto tex = std::make_shared<Texture>();

            if (!tex->LoadFromFile(fullPath.string(), ctx))
                continue;

            tex->SetId(mNextResourceID++);
            tex->SetPath(fullPath.string());
            tex->SetAlias(basePath + "_tex_" + suffix);
            manager.Add(tex);

            textureIds.push_back(tex->GetId());

            switch (type)
            {
            case aiTextureType_DIFFUSE:           mat->diffuseTexId = tex->GetId(); break;
            case aiTextureType_NORMALS:           mat->normalTexId = tex->GetId(); break;
            case aiTextureType_DIFFUSE_ROUGHNESS: mat->roughnessTexId = tex->GetId(); break;
            case aiTextureType_METALNESS:         mat->metallicTexId = tex->GetId(); break;
            }
        }
    }

    return textureIds;
}

std::shared_ptr<Model::Node> ResourceRegistry::ProcessNode(aiNode* ainode, const aiScene* scene, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes)
{
    auto node = std::make_shared<Model::Node>();
    node->name = ainode->mName.C_Str();

    aiMatrix4x4 aim = ainode->mTransformation;
    XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&aim)));
    XMStoreFloat4x4(&node->localTransform, xm);

    for (unsigned int i = 0; i < ainode->mNumMeshes; i++)
    {
        UINT meshIndex = ainode->mMeshes[i];
        node->meshes.push_back(loadedMeshes[meshIndex]);
    }

    for (unsigned int i = 0; i < ainode->mNumChildren; i++)
        node->children.push_back(ProcessNode(ainode->mChildren[i], scene, loadedMeshes));


    return node;
}

Skeleton ResourceRegistry::BuildSkeleton(const aiScene* scene)
{
    Skeleton skeleton;
    std::unordered_map<std::string, int> boneNameToIndex;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) 
    {
        aiMesh* aMesh = scene->mMeshes[m];
        for (unsigned int b = 0; b < aMesh->mNumBones; b++) 
        {
            aiBone* aBone = aMesh->mBones[b];
            std::string boneName = aBone->mName.C_Str();

            if (boneNameToIndex.count(boneName)) continue; 

            int idx = static_cast<int>(skeleton.BoneList.size());

            boneNameToIndex[boneName] = idx;
            
            skeleton.BoneNames.insert(boneName);

            Bone bone{};
            bone.name = boneName;
            bone.parentIndex = -1;

            XMMATRIX xm = XMMatrixTranspose(
                XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&aBone->mOffsetMatrix)));
            XMStoreFloat4x4(&bone.inverseBind, xm);

            skeleton.BoneList.push_back(bone);
        }
    }

    for (auto& [boneName, idx] : boneNameToIndex) 
    {
        aiNode* node = scene->mRootNode->FindNode(aiString(boneName.c_str()));
        if (node && node->mParent) 
        {
            std::string parentName = node->mParent->mName.C_Str();
            auto it = boneNameToIndex.find(parentName);
            if (it != boneNameToIndex.end()) 
            {
                skeleton.BoneList[idx].parentIndex = it->second;
            }
        }
    }

    skeleton.BuildNameToIndex();

    return skeleton;
}
