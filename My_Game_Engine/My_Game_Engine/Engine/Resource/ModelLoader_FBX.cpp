#include "ModelLoader_FBX.h"
#include "GameEngine.h"
#include "MaterialLoader.h"
#include "TextureLoader.h"
#include "MetaIO.h"
#include "DXMathUtils.h"

ModelLoader_FBX::ModelLoader_FBX()
{
}


bool ModelLoader_FBX::Load(const std::string& path, std::string_view alias, LoadResult& result)
{
    RendererContext ctx = GameEngine::Get().Get_UploadContext();
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    // --- FBX SDK 초기화 ---
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

    // --- Model 생성 ---
    auto model = std::make_shared<Model>();
    model->SetAlias(std::filesystem::path(path).stem().string());
    model->SetPath(path);
    rs->RegisterResource(model);
    result.modelId = model->GetId();

    // --- Material 처리 ---
    std::unordered_map<std::string, int> matNameCount;
    std::unordered_map<FbxSurfaceMaterial*, UINT> matMap;

    std::filesystem::path matDir = std::filesystem::path(path).parent_path() / "Materials";
    std::filesystem::create_directories(matDir);

    int matCount = scene->GetMaterialCount();
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
        auto mat = MaterialLoader::LoadOrReuse(ctx, matFilePath, uniqueName, path);
        if (!mat) continue;

        mat->SetGUID(MetaIO::CreateGUID(path, uniqueName));

        if (!std::filesystem::exists(matFilePath))
        {
            mat->FromFbxSDK(fbxMat);
            auto texIds = TextureLoader::LoadFromFbx(ctx, fbxMat, path, mat);
            result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
            mat->SaveToFile(matFilePath);
        }

        rs->RegisterResource(mat);
        matMap[fbxMat] = mat->GetId();
        result.materialIds.push_back(mat->GetId());
    }

    // --- Mesh + Node 계층 ---
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;
    if (scene->GetRootNode())
        model->SetRoot(ProcessNode(ctx, scene->GetRootNode(), matMap, path, loadedMeshes));

    // --- Skeleton ---
    Skeleton skeleton = BuildSkeleton(scene);
    model->SetSkeleton(skeleton);

    // --- Meta 저장 ---
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

    // Texture 등록
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

    // --- Local Transform ---
    FbxAMatrix local = fbxNode->EvaluateLocalTransform();
    XMFLOAT4X4 m;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            m.m[r][c] = static_cast<float>(local.Get(r, c));

    XMMATRIX xm = XMMatrixTranspose(XMLoadFloat4x4(&m));
    XMStoreFloat4x4(&node->localTransform, xm);

    // --- Mesh 생성 ---
    if (auto mesh = CreateMeshFromNode(fbxNode, matMap, path))
    {
        node->meshes.push_back(mesh);
        loadedMeshes.push_back(mesh);
    }

    // --- 자식 노드 처리 ---
    int childCount = fbxNode->GetChildCount();
    for (int i = 0; i < childCount; i++)
    {
        auto child = ProcessNode(ctx, fbxNode->GetChild(i), matMap, path, loadedMeshes);
        if (child) node->children.push_back(child);
    }

    return node;
}

static std::string BuildNodePath(FbxNode* node)
{
    if (!node) return "";

    std::string path = node->GetName();
    FbxNode* parent = node->GetParent();
    while (parent && parent->GetParent())
    {
        path = std::string(parent->GetName()) + "/" + path;
        parent = parent->GetParent();
    }
    return path;
}

std::shared_ptr<Mesh> ModelLoader_FBX::CreateMeshFromNode(
    FbxNode* fbxNode,
    std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
    const std::string& path)
{
    if (!fbxNode) return nullptr;
    FbxMesh* fbxMesh = fbxNode->GetMesh();
    if (!fbxMesh) return nullptr;

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    auto mesh = std::make_shared<Mesh>();
    mesh->FromFbxSDK(fbxMesh);

    std::string nodePath = BuildNodePath(fbxNode);
    std::string baseName = fbxNode->GetName();
    if (baseName.empty())
        baseName = nodePath;

    mesh->SetAlias(baseName);

    mesh->SetGUID(MetaIO::CreateGUID(path, baseName));

    const std::string uniqueKey = MakeSubresourcePath(path, "mesh", nodePath);
    mesh->SetPath(uniqueKey);

    // --- Submesh 분할 (Material Slot 기준) ---
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

    rs->RegisterResource(mesh);
    return mesh;
}


Skeleton ModelLoader_FBX::BuildSkeleton(FbxScene* fbxScene)
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

                Bone bone{};
                bone.name = boneName;
                bone.parentIndex = -1;

                FbxAMatrix bindMatrix;
                cluster->GetTransformLinkMatrix(bindMatrix);

                XMFLOAT4X4 m;
                for (int r = 0; r < 4; r++)
                    for (int c2 = 0; c2 < 4; c2++)
                        m.m[r][c2] = static_cast<float>(bindMatrix.Get(r, c2));
                bone.inverseBind = Matrix4x4::Transpose(m);

                boneNameToIndex[boneName] = (int)skeleton.BoneList.size();
                skeleton.BoneList.push_back(bone);
            }
        }
    }

    // 부모 관계 설정
    for (auto& [boneName, idx] : boneNameToIndex)
    {
        FbxNode* boneNode = fbxScene->FindNodeByName(boneName.c_str());
        if (boneNode && boneNode->GetParent())
        {
            std::string parentName = boneNode->GetParent()->GetName();
            auto it = boneNameToIndex.find(parentName);
            if (it != boneNameToIndex.end())
                skeleton.BoneList[idx].parentIndex = it->second;
        }
    }

    skeleton.BuildNameToIndex();
    return skeleton;
}
