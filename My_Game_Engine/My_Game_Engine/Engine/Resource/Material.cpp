#include "Material.h"
#include "MetaIO.h"
#include "GameEngine.h"

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

bool Material::LoadFromFile(std::string_view path, const RendererContext& ctx)
{
    std::ifstream ifs(path.data());
    if (!ifs.is_open())
    {
        OutputDebugStringA(("[Material] Failed to open: " + std::string(path) + "\n").c_str());
        return false;
    }

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
        if (arr.Size() > 2)
            albedoColor = XMFLOAT3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    if (doc.HasMember("metallic")) metallic = doc["metallic"].GetFloat();
    if (doc.HasMember("roughness")) roughness = doc["roughness"].GetFloat();

    if (doc.HasMember("textures"))
    {
        auto& rm = *GameEngine::Get().GetResourceManager();
        const auto& texObj = doc["textures"];
        auto LoadTex = [&](const char* key, UINT& texId, UINT& texSlot)
            {
                if (!texObj.HasMember(key)) return;
                const auto& texEntry = texObj[key];

                std::shared_ptr<Texture> tex = nullptr;

                if (texEntry.HasMember("guid"))
                    tex = rm.GetByGUID<Texture>(texEntry["guid"].GetString());

                if (!tex && texEntry.HasMember("path"))
                {
                    std::string texPath = texEntry["path"].GetString();
                    tex = std::make_shared<Texture>();
                    tex->LoadFromFile(texPath, ctx);
                    tex->SetPath(texPath);
                    rm.Add(tex);
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


bool Material::SaveToFile(const std::string& outputPath) const
{
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // --- 기본 정보 ---
    doc.AddMember("guid", Value(GetGUID().c_str(), alloc), alloc);
    doc.AddMember("name", Value(GetAlias().data(), alloc), alloc);
    std::string shader = shaderName.empty() ? "StandardPBR" : shaderName;
    doc.AddMember("shader", Value(shader.c_str(), alloc), alloc);

    // --- PBR 속성 ---
    Value colorArr(kArrayType);
    colorArr.PushBack(albedoColor.x, alloc);
    colorArr.PushBack(albedoColor.y, alloc);
    colorArr.PushBack(albedoColor.z, alloc);
    doc.AddMember("albedoColor", colorArr, alloc);
    doc.AddMember("metallic", metallic, alloc);
    doc.AddMember("roughness", roughness, alloc);

    // --- 텍스처 정보 (경로 + GUID) ---
    Value texObj(kObjectType);
    auto resMgr = GameEngine::Get().GetResourceManager();
    auto WriteTex = [&](const char* key, UINT texId)
        {
            if (texId == Engine::INVALID_ID)    return;

            auto tex = resMgr->GetById<Texture>(texId);
            if (!tex)   return;

            Value texEntry(kObjectType);

            std::string texPath = tex->GetPath().data();
            std::string texGuid = tex->GetGUID();

            texEntry.AddMember("path", Value(texPath.c_str(), static_cast<SizeType>(texPath.size()), alloc), alloc);
            texEntry.AddMember("guid", Value(texGuid.c_str(), static_cast<SizeType>(texGuid.size()), alloc), alloc);

            texObj.AddMember(Value(key, alloc), texEntry, alloc);
        };

    WriteTex("albedo", diffuseTexId);
    WriteTex("normal", normalTexId);
    WriteTex("roughness", roughnessTexId);
    WriteTex("metallic", metallicTexId);

    doc.AddMember("textures", texObj, alloc);


    std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

    std::ofstream ofs(outputPath, std::ios::trunc);
    if (!ofs.is_open()) return false;

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    ofs << buffer.GetString();
    ofs.close();


    auto self = const_cast<Material*>(this);
    self->SetPath(outputPath);
    MetaIO::EnsureResourceGUID(std::shared_ptr<Game_Resource>(
        std::const_pointer_cast<Material>(std::static_pointer_cast<const Material>(
            std::shared_ptr<const Material>(this, [](const Material*) {})))));

    MetaIO::SaveSimpleMeta(std::make_shared<Material>(*this));
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