#include "ResourceRegistry.h"
#include "DXMathUtils.h"
#include "MetaIO.h"

LoadResult ResourceRegistry::Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx)
{
    LoadResult result;

    if (LoadWithAssimp(manager, path, alias, ctx, result))  return result;
    if (LoadWithFbxSdk(manager, path, alias, ctx, result))  return result;


    OutputDebugStringA(("[Loader] Failed to load resource: " + path + "\n").c_str());
    return result;
}


bool ResourceRegistry::LoadWithAssimp(ResourceManager& manager, const std::string& path, std::string_view aalias, const RendererContext& ctx, LoadResult& result)
{
    LoadResult load_result;

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
        return false;
    }

    auto model = std::make_shared<Model>();

    if (scene->mName.length > 0)
        model->SetAlias(scene->mName.C_Str());
    else
        model->SetAlias(std::filesystem::path(path).stem().string());

    model->SetId(mNextResourceID++);
    model->SetPath(path);

    manager.Add(model);
    load_result.modelId = model->GetId();

    //---------------------- Meta List ------------------------
    std::vector<SubResourceMeta> subMetaList;

    // ---------- Name maps for duplicate handling ----------
    std::unordered_map<std::string, int> matNameCount;
    std::unordered_map<std::string, int> meshNameCount;

    std::vector<UINT> matIdTable(scene->mNumMaterials);

    // ---------- Materials ----------
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        auto mat = std::make_shared<Material>();
        mat->FromAssimp(scene->mMaterials[i]);

        aiString matName = scene->mMaterials[i]->GetName();

        std::string baseName;
        if (matName.length > 0)
            baseName = matName.C_Str();
        else
            baseName = "Material_" + std::to_string(i);

        // --- Duplicate name handling for Material ---
        std::string uniqueName = baseName;
        if (matNameCount.find(baseName) != matNameCount.end()) 
        {
            int count = ++matNameCount[baseName];
            uniqueName = baseName + "_" + std::to_string(count);
        }
        else 
            matNameCount[baseName] = 0;
        

        mat->SetAlias(uniqueName);
        mat->SetId(mNextResourceID++);
        mat->SetPath(path);
        manager.Add(mat);

        std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
        std::filesystem::create_directories(matDir);
        std::string matFilePath = (matDir / (uniqueName + ".mat")).string();
        mat->SaveToFile(matFilePath);

        subMetaList.push_back({ uniqueName, "Material", mat->GetGUID() });



        load_result.materialIds.push_back(mat->GetId());
        matIdTable[i] = mat->GetId();

        auto texIds = LoadMaterialTextures_Assimp(manager, scene->mMaterials[i], path, mat, ctx);
        load_result.textureIds.insert(load_result.textureIds.end(), texIds.begin(), texIds.end());
    }

    // ---------- Meshes ----------
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        auto mesh = std::make_shared<Mesh>();
        mesh->FromAssimp(scene->mMeshes[i]);

        std::string baseName = scene->mMeshes[i]->mName.C_Str();
        if (baseName.empty())
            baseName = "Mesh_" + std::to_string(i);

        // --- Duplicate name handling for Mesh ---
        std::string uniqueName = baseName;
        if (meshNameCount.find(baseName) != meshNameCount.end()) 
        {
            int count = ++meshNameCount[baseName];
            uniqueName = baseName + "_" + std::to_string(count);
        }
        else 
            meshNameCount[baseName] = 0;

        mesh->SetAlias(uniqueName);
        mesh->SetId(mNextResourceID++);
        mesh->SetPath(path);

        Mesh::Submesh sub{};
        sub.indexCount = static_cast<UINT>(mesh->indices.size());
        sub.startIndexLocation = 0;
        sub.baseVertexLocation = 0;

        UINT matIndex = scene->mMeshes[i]->mMaterialIndex;
        sub.materialId = (matIndex < matIdTable.size()) ? matIdTable[matIndex] : Engine::INVALID_ID;
        mesh->submeshes.push_back(sub);

        manager.Add(mesh);
        subMetaList.push_back({ std::string(mesh->GetAlias()), "Mesh", mesh->GetGUID() });
        loadedMeshes.push_back(mesh);


        load_result.meshIds.push_back(mesh->GetId());
        model->AddMesh(mesh);
    }

    if (scene->mRootNode)
        model->SetRoot(ProcessNode(scene->mRootNode, scene, loadedMeshes));

    // ---------- Skeleton ----------
    Skeleton skeleton = BuildSkeleton_Assimp(scene);
    model->SetSkeleton(skeleton);

    FbxMeta meta;
    meta.guid = model->GetGUID();
    meta.path = path;
    meta.sub_resources = std::move(subMetaList);
    MetaIO::SaveFbxMeta(meta);

    result = load_result;

    return true;
}

std::vector<UINT> ResourceRegistry::LoadMaterialTextures_Assimp(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx)
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

Skeleton ResourceRegistry::BuildSkeleton_Assimp(const aiScene* scene)
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

bool ResourceRegistry::LoadWithFbxSdk(
    ResourceManager& manager,
    const std::string& path,
    std::string_view aalias,
    const RendererContext& ctx,
    LoadResult& result)
{
    LoadResult load_result;

    // --- FBX Import ---
    FbxManager* fbxManager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(ios);

    FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "");
    if (!fbxImporter->Initialize(path.c_str(), -1, fbxManager->GetIOSettings()))
    {
        OutputDebugStringA(("[FBX SDK] Failed: " + path + "\n").c_str());
        fbxImporter->Destroy();
        fbxManager->Destroy();
        return false;
    }

    FbxScene* fbxScene = FbxScene::Create(fbxManager, "scene");
    fbxImporter->Import(fbxScene);
    fbxImporter->Destroy();

    // --- Model ---
    auto model = std::make_shared<Model>();
    model->SetAlias(std::filesystem::path(path).stem().string());
    model->SetId(mNextResourceID++);
    model->SetPath(path);

    manager.Add(model);
    load_result.modelId = model->GetId();

    //---------------------- Meta List ------------------------
    std::vector<SubResourceMeta> subMetaList;

    //--------------------- Material -----------------------
    std::unordered_map<std::string, int> matNameCount;
    std::unordered_map<FbxSurfaceMaterial*, UINT> matMap; 

    int matCount = fbxScene->GetMaterialCount();
    for (int i = 0; i < matCount; i++)
    {
        FbxSurfaceMaterial* fbxMat = fbxScene->GetMaterial(i);
        if (!fbxMat) continue;

        auto mat = std::make_shared<Material>();
        mat->FromFbxSDK(fbxMat);

        std::string baseName = fbxMat->GetName();
        if (baseName.empty()) baseName = "Material_" + std::to_string(i);

        std::string uniqueName = baseName;
        if (matNameCount.find(baseName) != matNameCount.end())
        {
            int count = ++matNameCount[baseName];
            uniqueName = baseName + "_" + std::to_string(count);
        }
        else matNameCount[baseName] = 0;

        mat->SetAlias(uniqueName);
        mat->SetId(mNextResourceID++);
        mat->SetPath(path);
        manager.Add(mat);

        std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
        std::filesystem::create_directories(matDir);
        std::string matFilePath = (matDir / (uniqueName + ".mat")).string();
        mat->SaveToFile(matFilePath);

        subMetaList.push_back({ uniqueName, "Material", mat->GetGUID() });
        matMap[fbxMat] = mat->GetId();
        load_result.materialIds.push_back(mat->GetId());

        // --- Texture ---
        auto texIds = LoadMaterialTexturesFbx(manager, fbxMat, path, mat, ctx);
        load_result.textureIds.insert(load_result.textureIds.end(), texIds.begin(), texIds.end());
    }

    //--------------------- Mesh + Node Hierarchy -----------------------
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    if (fbxScene->GetRootNode())
        model->SetRoot(ProcessFbxNode(fbxScene->GetRootNode(), manager, matMap, path, ctx, loadedMeshes));

    //--------------------- Skeleton -----------------------
    Skeleton skeleton = BuildSkeletonFbx(fbxScene);
    model->SetSkeleton(skeleton);


    // --- Save FBX Meta (single file) ---
    FbxMeta meta;
    meta.guid = model->GetGUID();
    meta.path = path;
    meta.sub_resources = std::move(subMetaList);

    MetaIO::SaveFbxMeta(meta);

    result = load_result;

    fbxManager->Destroy();
    return true;
}


std::shared_ptr<Model::Node> ResourceRegistry::ProcessFbxNode(
    FbxNode* fbxNode,
    ResourceManager& manager,
    std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
    const std::string& path,
    const RendererContext& ctx,
    std::vector<std::shared_ptr<Mesh>>& loadedMeshes)
{
    if (!fbxNode) return nullptr;

    auto node = std::make_shared<Model::Node>();
    node->name = fbxNode->GetName();

    FbxAMatrix local = fbxNode->EvaluateLocalTransform();
    XMFLOAT4X4 m;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            m.m[r][c] = static_cast<float>(local.Get(r, c));

    XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(&m));
    XMStoreFloat4x4(&node->localTransform, xm);

    if (auto mesh = CreateMeshFromFbxNode(fbxNode, manager, matMap, path, ctx))
    {
        node->meshes.push_back(mesh);
        loadedMeshes.push_back(mesh);
    }

    // --- Children ---
    int childCount = fbxNode->GetChildCount();
    for (int i = 0; i < childCount; i++)
    {
        auto child = ProcessFbxNode(fbxNode->GetChild(i), manager, matMap, path, ctx, loadedMeshes);
        if (child) node->children.push_back(child);
    }

    return node;
}


std::vector<UINT> ResourceRegistry::LoadMaterialTexturesFbx(
    ResourceManager& manager,
    FbxSurfaceMaterial* fbxMat,
    const std::string& basePath,
    std::shared_ptr<Material>& mat,
    const RendererContext& ctx)
{
    std::vector<UINT> textureIds;

    auto LoadTex = [&](const char* propName, UINT& texId, UINT& texSlot)
        {
            FbxProperty prop = fbxMat->FindProperty(propName);
            if (!prop.IsValid()) return;

            int texCount = prop.GetSrcObjectCount<FbxFileTexture>();
            if (texCount <= 0) return;

            FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
            if (!tex) return;

            std::filesystem::path baseDir = std::filesystem::path(basePath).parent_path();
            std::filesystem::path texPath = tex->GetFileName();

            std::filesystem::path relPath = texPath.filename();
            std::filesystem::path fullPath = baseDir / relPath;

            if (auto existTex = manager.GetByPath<Texture>(fullPath.string()))
            {
                texId = existTex->GetId();
                texSlot = existTex->GetSlot();
                textureIds.push_back(texId);
                return;
            }

            auto newTex = std::make_shared<Texture>();
            if (!newTex->LoadFromFile(fullPath.string(), ctx))
                return;

            newTex->SetId(mNextResourceID++);
            newTex->SetPath(fullPath.string());
            newTex->SetAlias(fullPath.stem().string());
            manager.Add(newTex);

            texId = newTex->GetId();
            texSlot = newTex->GetSlot();
            textureIds.push_back(texId);
        };

    // FBX 기본 속성 이름 (DX12 PBR 기준)
    LoadTex(FbxSurfaceMaterial::sDiffuse, mat->diffuseTexId, mat->diffuseTexSlot);
    LoadTex(FbxSurfaceMaterial::sNormalMap, mat->normalTexId, mat->normalTexSlot);
    LoadTex(FbxSurfaceMaterial::sSpecular, mat->roughnessTexId, mat->roughnessTexSlot); // roughness 대체
    LoadTex(FbxSurfaceMaterial::sReflection, mat->metallicTexId, mat->metallicTexSlot);  // metallic 대체

    return textureIds;
}


std::shared_ptr<Mesh> ResourceRegistry::CreateMeshFromFbxNode(
    FbxNode* fbxNode,
    ResourceManager& manager,
    std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
    const std::string& path,
    const RendererContext& ctx)
{
    if (!fbxNode) return nullptr;
    FbxMesh* fbxMesh = fbxNode->GetMesh();
    if (!fbxMesh) return nullptr;

    auto mesh = std::make_shared<Mesh>();
    mesh->FromFbxSDK(fbxMesh); // Flatten only

    std::string baseName = fbxNode->GetName();
    if (baseName.empty())
        baseName = "Mesh_" + std::to_string(mNextResourceID);

    mesh->SetAlias(baseName);
    mesh->SetId(mNextResourceID++);
    mesh->SetPath(path);

    // --- Submesh by material ---
    int polyCount = fbxMesh->GetPolygonCount();
    FbxGeometryElementMaterial* matElem = fbxMesh->GetElementMaterial();

    if (matElem && fbxNode->GetMaterialCount() > 0)
    {
        std::unordered_map<int, std::vector<UINT>> polyByMatSlot;

        int idx = 0;
        for (int p = 0; p < polyCount; p++)
        {
            int matSlot = matElem->GetIndexArray().GetAt(p);
            int vCountInPoly = fbxMesh->GetPolygonSize(p);

            for (int v = 0; v < vCountInPoly; v++)
            {
                polyByMatSlot[matSlot].push_back(mesh->indices[idx]);
                idx++;
            }
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

    manager.Add(mesh);
    return mesh;
}




Skeleton ResourceRegistry::BuildSkeletonFbx(FbxScene* fbxScene)
{
    Skeleton skeleton;
    if (!fbxScene) return skeleton;

    std::unordered_map<std::string, int> boneNameToIndex;


    int nodeCount = fbxScene->GetNodeCount();
    for (int n = 0; n < nodeCount; n++)
    {
        FbxNode* node = fbxScene->GetNode(n);
        if (!node) continue;

        FbxMesh* fbxMesh = node->GetMesh();
        if (!fbxMesh) continue;

        int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
        for (int s = 0; s < skinCount; s++)
        {
            FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(s, FbxDeformer::eSkin));
            if (!skin) continue;

            int clusterCount = skin->GetClusterCount();
            for (int c = 0; c < clusterCount; c++)
            {
                FbxCluster* cluster = skin->GetCluster(c);
                if (!cluster) continue;

                FbxNode* linkNode = cluster->GetLink(); 
                if (!linkNode) continue;

                std::string boneName = linkNode->GetName();
                if (boneNameToIndex.count(boneName)) continue;

                // --- Bone ---
                int idx = static_cast<int>(skeleton.BoneList.size());
                boneNameToIndex[boneName] = idx;
                skeleton.BoneNames.insert(boneName);

                Bone bone{};
                bone.name = boneName;
                bone.parentIndex = -1;

                // --- Inverse Bind Pose ---
                FbxAMatrix bindMatrix;
                cluster->GetTransformLinkMatrix(bindMatrix);

                XMFLOAT4X4 m;
                for (int r = 0; r < 4; r++)
                {
                    for (int c = 0; c < 4; c++)
                    {
                        m.m[r][c] = static_cast<float>(bindMatrix.Get(r, c));
                    }
                }
                bone.inverseBind = Matrix4x4::Transpose(m);
            }
        }
    }


    if (!skeleton.BoneList.empty())
    {
        for (auto& [boneName, idx] : boneNameToIndex)
        {
            FbxNode* boneNode = fbxScene->FindNodeByName(boneName.c_str());
            if (boneNode && boneNode->GetParent())
            {
                std::string parentName = boneNode->GetParent()->GetName();
                auto it = boneNameToIndex.find(parentName);
                if (it != boneNameToIndex.end())
                {
                    skeleton.BoneList[idx].parentIndex = it->second;
                }
            }
        }
    }


    skeleton.BuildNameToIndex();

    return skeleton;
}
