#pragma once


enum class FileCategory
{
    ComplexModel,
    FBX,
    Clip,
    Skeleton,
    Model_Avatar,
    Texture,
    Material,
    AvatarMask,
	RawData,
    Unknown
};


static std::string ExtractFileName(const std::string& path)
{
    std::string base_path = path;

    size_t hash_pos = path.find('#');
    if (hash_pos != std::string::npos)
    {
        base_path = path.substr(0, hash_pos);
    }

    return std::filesystem::path(base_path).stem().string();
}

static std::string GetPhysicalFilePath(const std::string& path)
{
    std::string base_path = path;
    size_t hash_pos = path.find('#');
    if (hash_pos != std::string::npos)
    {
        base_path = path.substr(0, hash_pos);
    }

    return std::filesystem::path(base_path).lexically_normal().string();
}

static std::string NormalizeFilePath(const std::string& path)
{
    std::string base_path = path;
    size_t hash_pos = path.find('#');
    if (hash_pos != std::string::npos)
    {
        base_path = path.substr(0, hash_pos);
    }

    std::filesystem::path fs_path(base_path);
    return fs_path.lexically_normal().string();
}

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


    if (ext == ".raw")
        return FileCategory::RawData;

    if (ext == ".mat")
        return FileCategory::Material;

    if(ext == ".anim")
		return FileCategory::Clip;

    if(ext == ".skel")
		return FileCategory::Skeleton;

    if(ext == ".avatar")
		return FileCategory::Model_Avatar;

    if (ext == ".mask")
        return FileCategory::AvatarMask;

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
    AnimationClip,
    AvatarMask,
    TerrainData,
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
    bool mIsTemporary = false;

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
    void SetTemporary(bool temp) { mIsTemporary = temp; }

    UINT GetId() const { return resource_id; }
    const std::string& GetGUID() const { return GUID; }
    ResourceType Get_Type() const { return resource_type; }
    std::string GetAlias() const { return alias; }
    std::string GetPath() const { return file_path; }
    std::string GetPathCopy() const { return file_path; }
    virtual UINT GetSlot() const { return mSlot; }
    bool IsTemporary() const { return mIsTemporary; }

};