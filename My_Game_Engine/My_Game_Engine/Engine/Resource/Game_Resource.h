#pragma once


enum class FileCategory
{
    ComplexModel,
    FBX,
    Texture,
    Material,
    Unknown
};

static FileCategory DetectFileCategory(const std::string& path)
{
    std::string ext;
    try {
        ext = std::filesystem::path(path).extension().string();
    }
    catch (...) {
        return FileCategory::Unknown;
    }

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".fbx")
        return FileCategory::FBX;

    if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".dae")
        return FileCategory::ComplexModel;

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".tga" || ext == ".bmp" || ext == ".dds" || ext == ".hdr")
        return FileCategory::Texture;

    if (ext == ".mat")
        return FileCategory::Material;

    return FileCategory::Unknown;
}


struct RendererContext;

enum class ResourceType 
{
    Mesh,
    Material,
    Texture,
    Model,
    ModelAvatar,
    Skeleton,
    etc,
};

class Game_Resource 
{
private:
    UINT resource_id; // Resource ID

    ResourceType resource_type;
    std::string alias;
    std::string file_path;
    std::string GUID;
protected:
    UINT mSlot = UINT(-1); // Desrciptor Heap Index

public:
    Game_Resource(ResourceType new_resource_type = ResourceType::etc) : resource_type(new_resource_type) { resource_id = Engine::INVALID_ID; };
    virtual ~Game_Resource() = default;

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) = 0;
    virtual bool SaveToFile(const std::string& path) const = 0; 

    void SetId(UINT id) { resource_id = id; }
    void SetGUID(const std::string& guid) { GUID = guid; }
    void SetAlias(std::string a) { alias = a; }
    void SetPath(std::string p) { file_path = p.data(); }

    UINT GetId() const { return resource_id; }
    const std::string& GetGUID() const { return GUID; }
    ResourceType Get_Type() const { return resource_type; }
    std::string GetAlias() const { return alias; }
    std::string GetPath() const { return file_path; }
    std::string GetPathCopy() const { return file_path; }
    virtual UINT GetSlot() const { return mSlot; }

};