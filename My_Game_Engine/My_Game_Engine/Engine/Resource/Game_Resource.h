#pragma once
#include "pch.h"

class ResourceRegistry;

enum class ResourceType {
    Mesh,
    Material,
    Texture,
    etc,
};

class Game_Resource 
{
    friend class ResourceRegistry;
private:
    UINT resource_id;
    ResourceType resource_type;

    std::string alias;
    std::string file_path;

protected:
    void SetId(UINT id) { resource_id = id; }
    void SetAlias(std::string_view a) { alias = a; }
    void SetPath(std::string_view p) { file_path = p; }

public:
    Game_Resource(ResourceType new_resource_type = ResourceType::etc) : resource_type(new_resource_type) {};
    virtual ~Game_Resource() = default;
    virtual bool LoadFromFile(std::string_view path) = 0;

    UINT GetId() const { return resource_id; }
    std::string_view GetAlias() const { return alias; }
    std::string_view GetPath() const { return file_path; }
};
