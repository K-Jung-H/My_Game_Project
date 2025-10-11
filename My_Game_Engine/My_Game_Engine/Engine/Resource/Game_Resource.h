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

public:
    Game_Resource(ResourceType new_resource_type = ResourceType::etc) : resource_type(new_resource_type) { resource_id = Engine::INVALID_ID; };
    virtual ~Game_Resource() = default;
    virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx) = 0;


    void SetId(UINT id) { resource_id = id; }
    void SetGUID(const std::string& guid) { GUID = guid; }
    void SetAlias(std::string_view a) { alias = a; }
    void SetPath(std::string_view p) { file_path = p.data(); }

    UINT GetId() const { return resource_id; }
    const std::string& GetGUID() const { return GUID; }
    ResourceType Get_Type() const { return resource_type; }
    std::string_view GetAlias() const { return alias; }
    std::string_view GetPath() const { return file_path; }
    std::string GetPathCopy() const { return file_path; }
    virtual UINT GetSlot() const { return mSlot; }

};