#include "ModelLoader_Assimp.h"
#include "MaterialLoader.h"
#include "TextureLoader.h"
#include "MetaIO.h"
#include "GameEngine.h"


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
        aiProcess_GenNormals);

    if (!ai_scene)
    {
        OutputDebugStringA(("[Assimp] Failed to load: " + path + "\n").c_str());
        return false;
    }

    auto model = std::make_shared<Model>();
    model->SetAlias(std::filesystem::path(path).stem().string());
    model->SetPath(path);
    rs->RegisterResource(model);
    result.modelId = model->GetId();

    std::unordered_map<std::string, int> matNameCount;
    std::vector<UINT> matIdTable(ai_scene->mNumMaterials);
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

        auto mat = MaterialLoader::LoadOrReuse(ctx, matFilePath, uniqueName, path);
        if (!mat) continue;

        if (!std::filesystem::exists(matFilePath))
        {
            mat->FromAssimp(aiMat);
            auto texIds = TextureLoader::LoadFromAssimp(ctx, aiMat, path, mat);
            result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
            mat->SaveToFile(matFilePath);
        }

        rs->RegisterResource(mat);
        matIdTable[i] = mat->GetId();
        result.materialIds.push_back(mat->GetId());
    }

    if (ai_scene->mNumAnimations > 0 || ai_scene->HasMeshes())
    {
        auto skeletonRes = BuildSkeleton(ai_scene);
        std::string skelAlias = model->GetAlias() + "_Skeleton";
        skeletonRes->SetAlias(skelAlias);
        skeletonRes->SetPath(MakeSubresourcePath(path, "skeleton", skelAlias));
        skeletonRes->SetGUID(MetaIO::CreateGUID(skeletonRes->GetPath(), skelAlias));
        rs->RegisterResource(skeletonRes);
        model->SetSkeleton(skeletonRes);
    }

    std::unordered_map<std::string, int> nameCount;

    for (unsigned int i = 0; i < ai_scene->mNumMeshes; i++)
    {
        aiMesh* ai_mesh = ai_scene->mMeshes[i];
        const bool isSkinned = (ai_mesh->HasBones() && ai_scene->mNumAnimations > 0);

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
        mesh->SetGUID(MetaIO::CreateGUID(path, baseName));
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
        model->AddMesh(mesh);
    }

    if (ai_scene->mRootNode)
        model->SetRoot(ProcessNode(ai_scene->mRootNode, ai_scene));

    FbxMeta meta;
    meta.guid = model->GetGUID();
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

std::shared_ptr<Skeleton> ModelLoader_Assimp::BuildSkeleton(const aiScene* scene)
{
    auto skeletonRes = std::make_shared<Skeleton>();

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


            boneMap[name] = (int)skeletonRes->BoneList.size();
            skeletonRes->BoneList.push_back(bdata);
        }
    }

    skeletonRes->BuildNameToIndex();

    return skeletonRes;
}