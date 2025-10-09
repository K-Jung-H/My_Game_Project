#pragma once

struct RendererContext;
class ResourceRegistry;

enum class ResourceType 
{
    Mesh,
    Material,
    Texture,
    etc,
};

class Game_Resource 
{
    friend class ResourceRegistry;
    friend class ResourceManager;

private:
    UINT resource_id; // Resource ID

    ResourceType resource_type;
    std::string alias;
    std::string file_path;
    std::string GUID;
protected:
    UINT mSlot = UINT(-1); // Desrciptor Heap Index

protected:
    void SetId(UINT id) { resource_id = id; }
    void SetGUID(const std::string& guid) { GUID = guid; }
    void SetAlias(std::string_view a) { alias = a; }
    void SetPath(std::string_view p) { file_path = p; }
public:
    Game_Resource(ResourceType new_resource_type = ResourceType::etc) : resource_type(new_resource_type) { resource_id = Engine::INVALID_ID; };
    virtual ~Game_Resource() = default;
    virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx) = 0;

    UINT GetId() const { return resource_id; }
    const std::string& GetGUID() const { return GUID; }
    std::string_view GetAlias() const { return alias; }
    std::string_view GetPath() const { return file_path; }
    virtual UINT GetSlot() const { return mSlot; }

};

inline std::string GenerateGUID()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    uint64_t high = dis(gen);
    uint64_t low = dis(gen);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << high
        << std::setw(16) << std::setfill('0') << low;
    return ss.str();
}