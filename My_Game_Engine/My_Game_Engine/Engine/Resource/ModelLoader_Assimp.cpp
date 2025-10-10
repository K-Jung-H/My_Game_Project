#include "ModelLoader_Assimp.h"
#include "MaterialLoader.h"
#include "TextureLoader.h"
#include "MetaIO.h"


ModelLoader_Assimp::ModelLoader_Assimp()
{
}

bool ModelLoader_Assimp::Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx, LoadResult& result)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals);

    if (!scene)
    {
        OutputDebugStringA(("[Assimp] Failed to load: " + path + "\n").c_str());
        return false;
    }

    auto model = std::make_shared<Model>();
    model->SetAlias(std::filesystem::path(path).stem().string());
    model->SetId(ResourceRegistry::GenerateID());
    model->SetPath(path);
    manager.Add(model);
    result.modelId = model->GetId();

    // --- Material Ã³¸® ---
    std::unordered_map<std::string, int> matNameCount;
    std::vector<UINT> matIdTable(scene->mNumMaterials);
    std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
    std::filesystem::create_directories(matDir);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        aiString aiMatName;
        aiMat->Get(AI_MATKEY_NAME, aiMatName);

        std::string baseName = aiMatName.length > 0 ? aiMatName.C_Str() : "Material_" + std::to_string(i);
        std::string uniqueName = baseName + (matNameCount[baseName]++ > 0 ? "_" + std::to_string(matNameCount[baseName]) : "");

        std::string matFilePath = (matDir / (uniqueName + ".mat")).string();
        auto mat = MaterialLoader::LoadOrReuse(manager, ctx, matFilePath, uniqueName, path);
        if (!mat) continue;

        if (!std::filesystem::exists(matFilePath))
        {
            mat->FromAssimp(aiMat);
            auto texIds = TextureLoader::LoadFromAssimp(manager, aiMat, path, mat, ctx);
            result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
            mat->SaveToFile(matFilePath);
        }

        matIdTable[i] = mat->GetId();
        result.materialIds.push_back(mat->GetId());
    }

    // --- Mesh ---
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        auto mesh = std::make_shared<Mesh>();
        mesh->FromAssimp(scene->mMeshes[i]);

        std::string baseName = scene->mMeshes[i]->mName.C_Str();
        if (baseName.empty()) baseName = "Mesh_" + std::to_string(i);
        mesh->SetAlias(baseName);
        mesh->SetId(ResourceRegistry::GenerateID());
        mesh->SetPath(path);

        Mesh::Submesh sub{};
        sub.indexCount = (UINT)mesh->indices.size();
        sub.materialId = scene->mMeshes[i]->mMaterialIndex < matIdTable.size() ? matIdTable[scene->mMeshes[i]->mMaterialIndex] : Engine::INVALID_ID;
        mesh->submeshes.push_back(sub);

        manager.Add(mesh);
        loadedMeshes.push_back(mesh);
        result.meshIds.push_back(mesh->GetId());
        model->AddMesh(mesh);
    }

    // --- Hierarchy / Skeleton ---
    if (scene->mRootNode)
        model->SetRoot(ProcessNode(scene->mRootNode, scene, loadedMeshes));

    model->SetSkeleton(BuildSkeleton(scene));

    // --- Meta ---
    FbxMeta meta;
    meta.guid = model->GetGUID();
    meta.path = path;
    MetaIO::SaveFbxMeta(meta);

    return true;
}

std::shared_ptr<Model::Node> ModelLoader_Assimp::ProcessNode(aiNode* ainode, const aiScene* scene, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes)
{
    auto node = std::make_shared<Model::Node>();
    node->name = ainode->mName.C_Str();
    aiMatrix4x4 aim = ainode->mTransformation;
    XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&aim)));
    XMStoreFloat4x4(&node->localTransform, xm);

    for (unsigned int i = 0; i < ainode->mNumMeshes; i++)
        node->meshes.push_back(loadedMeshes[ainode->mMeshes[i]]);

    for (unsigned int i = 0; i < ainode->mNumChildren; i++)
        node->children.push_back(ProcessNode(ainode->mChildren[i], scene, loadedMeshes));

    return node;
}

Skeleton ModelLoader_Assimp::BuildSkeleton(const aiScene* scene)
{
    Skeleton skeleton;
    std::unordered_map<std::string, int> boneMap;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int b = 0; b < mesh->mNumBones; b++)
        {
            aiBone* bone = mesh->mBones[b];
            std::string name = bone->mName.C_Str();
            if (boneMap.count(name)) continue;

            Bone bdata{};
            bdata.name = name;
            bdata.parentIndex = -1;
            XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&bone->mOffsetMatrix)));
            XMStoreFloat4x4(&bdata.inverseBind, xm);

            boneMap[name] = (int)skeleton.BoneList.size();
            skeleton.BoneList.push_back(bdata);
        }
    }

    skeleton.BuildNameToIndex();
    return skeleton;
}
