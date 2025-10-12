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

    // --- 모델 생성 ---
    auto model = std::make_shared<Model>();
    model->SetAlias(std::filesystem::path(path).stem().string());
    model->SetPath(path);
    rs->RegisterResource(model);
    result.modelId = model->GetId();

    // --- Material 처리 ---
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

    // --- Mesh 처리 ---
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    for (unsigned int i = 0; i < ai_scene->mNumMeshes; i++)
    {
        auto mesh = std::make_shared<Mesh>();
        mesh->FromAssimp(ai_scene->mMeshes[i]);

        std::string baseName = ai_scene->mMeshes[i]->mName.C_Str();
        if (baseName.empty()) baseName = "Mesh_" + std::to_string(i);
        mesh->SetAlias(baseName);
        mesh->SetPath(path);

        Mesh::Submesh sub{};
        sub.indexCount = (UINT)mesh->indices.size();
        sub.materialId = ai_scene->mMeshes[i]->mMaterialIndex < matIdTable.size()
            ? matIdTable[ai_scene->mMeshes[i]->mMaterialIndex]
            : Engine::INVALID_ID;
        mesh->submeshes.push_back(sub);

        rs->RegisterResource(mesh);
        loadedMeshes.push_back(mesh);
        result.meshIds.push_back(mesh->GetId());
        model->AddMesh(mesh);
    }

    // --- 노드 계층 및 스켈레톤 구성 ---
    if (ai_scene->mRootNode)
        model->SetRoot(ProcessNode(ai_scene->mRootNode, ai_scene, loadedMeshes));

    model->SetSkeleton(BuildSkeleton(ai_scene));

    // --- Meta 정보 저장 ---
    FbxMeta meta;
    meta.guid = model->GetGUID();
    meta.path = path;

    // Mesh 등록
    for (auto& mesh : loadedMeshes)
    {
        SubResourceMeta s{};
        s.name = mesh->GetAlias();
        s.type = "MESH";
        s.guid = mesh->GetGUID();
        meta.sub_resources.push_back(s);
    }

    // Material 등록
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

    // Texture 등록
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

    // Meta 저장 (기존 파일 없을 경우만)
    if (!MetaIO::LoadFbxMeta(meta, path))
        MetaIO::SaveFbxMeta(meta);

    OutputDebugStringA(("[Assimp] Model loaded successfully: " + path + "\n").c_str());
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
