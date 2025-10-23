#include "Material.h"
#include "MetaIO.h"
#include "GameEngine.h"

Material Material::Get_Default()
{
    Material defaultMaterial;

    defaultMaterial.albedoColor = { 0.8f, 0.8f, 0.8f };
    defaultMaterial.roughness = 0.5f;
    defaultMaterial.metallic = 0.0f;

    defaultMaterial.diffuseTexId = Engine::INVALID_ID;
    defaultMaterial.normalTexId = Engine::INVALID_ID;
    defaultMaterial.roughnessTexId = Engine::INVALID_ID;
    defaultMaterial.metallicTexId = Engine::INVALID_ID;

    defaultMaterial.diffuseTexSlot = UINT_MAX;
    defaultMaterial.normalTexSlot = UINT_MAX;
    defaultMaterial.roughnessTexSlot = UINT_MAX;
    defaultMaterial.metallicTexSlot = UINT_MAX;

    return defaultMaterial;
}

Material::Material() : Game_Resource(ResourceType::Material)
{
    diffuseTexId = Engine::INVALID_ID;
    normalTexId = Engine::INVALID_ID;
    roughnessTexId = Engine::INVALID_ID;
    metallicTexId = Engine::INVALID_ID;

    diffuseTexSlot = UINT_MAX;
    normalTexSlot = UINT_MAX;
    roughnessTexSlot = UINT_MAX;
    metallicTexSlot = UINT_MAX;

    albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
    roughness = 0.5f;
    metallic = 0.0f;

    shaderName = "None";
}


bool Material::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::ifstream ifs(path.data());
    if (!ifs.is_open())
    {
        OutputDebugStringA(("[Material] Failed to open: " + std::string(path) + "\n").c_str());
        return false;
    }

    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    Document doc;
    if (doc.Parse(json.c_str()).HasParseError())
        return false;

    if (doc.HasMember("guid")) SetGUID(doc["guid"].GetString());
    if (doc.HasMember("name")) SetAlias(doc["name"].GetString());
    if (doc.HasMember("shader")) shaderName = doc["shader"].GetString();

    if (doc.HasMember("albedoColor") && doc["albedoColor"].IsArray())
    {
        const auto& arr = doc["albedoColor"].GetArray();
        if (arr.Size() >= 3)
            albedoColor = XMFLOAT3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    if (doc.HasMember("metallic"))  metallic = doc["metallic"].GetFloat();
    if (doc.HasMember("roughness")) roughness = doc["roughness"].GetFloat();

    if (doc.HasMember("textures") && doc["textures"].IsObject())
    {
        const auto& texObj = doc["textures"];
        auto LoadTex = [&](const char* key, UINT& texId, UINT& texSlot)
            {
                if (!texObj.HasMember(key)) return;
                const auto& texEntry = texObj[key];
                std::shared_ptr<Texture> tex = nullptr;

                // 1GUID 우선 검색
                if (texEntry.HasMember("guid"))
                    tex = rs->GetByGUID<Texture>(texEntry["guid"].GetString());

                // 2GUID가 없거나 캐시 미존재 → 경로 기반 로드
                if (!tex && texEntry.HasMember("path"))
                {
                    std::string texPath = texEntry["path"].GetString();
                    tex = rs->GetByPath<Texture>(texPath);

                    if (!tex)
                    {
                        tex = std::make_shared<Texture>();
                        if (!tex->LoadFromFile(texPath, ctx))
                        {
                            OutputDebugStringA(("[Material] Texture load failed: " + texPath + "\n").c_str());
                            return;
                        }

                        tex->SetPath(texPath);
                        tex->SetAlias(std::filesystem::path(texPath).stem().string());

                        rs->RegisterResource(tex);
                    }
                }

                if (tex)
                {
                    texId = tex->GetId();
                    texSlot = tex->GetSlot();
                }
            };

        LoadTex("albedo", diffuseTexId, diffuseTexSlot);
        LoadTex("normal", normalTexId, normalTexSlot);
        LoadTex("roughness", roughnessTexId, roughnessTexSlot);
        LoadTex("metallic", metallicTexId, metallicTexSlot);
    }

    SetPath(path);
    return true;
}

bool Material::SaveToFile(const std::string& path) const
{
    ResourceSystem* rsm = GameEngine::Get().GetResourceSystem();

    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // --- 기본 메타 ---
    doc.AddMember("guid", rapidjson::Value(GetGUID().c_str(), alloc), alloc);
    doc.AddMember("name", rapidjson::Value(GetAlias().c_str(), alloc), alloc);
    doc.AddMember("shader", rapidjson::Value(shaderName.c_str(), alloc), alloc);

    // --- 물리 속성 ---
    rapidjson::Value albedoArr(rapidjson::kArrayType);
    albedoArr.PushBack(albedoColor.x, alloc);
    albedoArr.PushBack(albedoColor.y, alloc);
    albedoArr.PushBack(albedoColor.z, alloc);
    doc.AddMember("albedoColor", albedoArr, alloc);

    doc.AddMember("metallic", metallic, alloc);
    doc.AddMember("roughness", roughness, alloc);

    // --- 텍스처 GUID/Path 저장 ---
    rapidjson::Value texObj(rapidjson::kObjectType);

    auto SaveTex = [&](const char* key, UINT texId)
        {
            if (texId == 0 || texId == Engine::INVALID_ID)
                return;

            if (auto tex = rsm->GetById<Texture>(texId))
            {
                rapidjson::Value texEntry(rapidjson::kObjectType);
                texEntry.AddMember("guid", rapidjson::Value(tex->GetGUID().c_str(), alloc), alloc);
                texEntry.AddMember("path", rapidjson::Value(tex->GetPath().c_str(), alloc), alloc);
                texObj.AddMember(rapidjson::Value(key, alloc), texEntry, alloc);
            }
        };

    SaveTex("albedo", diffuseTexId);
    SaveTex("normal", normalTexId);
    SaveTex("roughness", roughnessTexId);
    SaveTex("metallic", metallicTexId);

    doc.AddMember("textures", texObj, alloc);

    // --- 파일로 출력 ---
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs.is_open())
    {
        OutputDebugStringA(("[Material] Failed to save: " + path + "\n").c_str());
        return false;
    }

    ofs << buffer.GetString();
    ofs.close();

    OutputDebugStringA(("[Material] Saved: " + path + "\n").c_str());
    return true;
}

void Material::FromAssimp(const aiMaterial* material)
{
    aiColor4D color;
    float shininess = 0.0f;
    float metalness = 0.0f;


    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
        albedoColor = XMFLOAT3(color.r, color.g, color.b);
    else
        albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);


    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess))
        roughness = 1.0f - std::min(shininess / 256.0f, 1.0f);
    else
        roughness = 0.5f;


    if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
        metallic = metalness;
    else
        metallic = 0.0f;

}

void Material::FromFbxSDK(const FbxSurfaceMaterial* fbxMat)
{
    if (!fbxMat) return;

    albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
    roughness = 0.5f;
    metallic = 0.0f;

    // --- Diffuse Color ---
    if (fbxMat->GetClassId().Is(FbxSurfaceLambert::ClassId))
    {
        auto lambert = (FbxSurfaceLambert*)fbxMat;
        FbxDouble3 diffuse = lambert->Diffuse.Get();
        albedoColor = XMFLOAT3((float)diffuse[0], (float)diffuse[1], (float)diffuse[2]);
    }

    // --- Shininess -> Roughness  ---
    if (fbxMat->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        auto phong = (FbxSurfacePhong*)fbxMat;
        float shininess = (float)phong->Shininess.Get();
        roughness = 1.0f - std::min(shininess / 256.0f, 1.0f);
    }

    // FBX 기본 머티리얼에는 Metallic 정보가 없음
    // 필요 시, 확장 속성 찾아야 함
    metallic = 0.0f;


}