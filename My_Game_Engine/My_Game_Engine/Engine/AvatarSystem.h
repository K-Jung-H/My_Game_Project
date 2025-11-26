#pragma once

enum class DefinitionType
{
    None,
    Humanoid,
    Animal,
};

static DefinitionType StringToDefinitionType(const std::string& s)
{
    if (s == "Humanoid") return DefinitionType::Humanoid;
    if (s == "Animal") return DefinitionType::Animal;
    return DefinitionType::None;
}

struct BoneDefinition
{
    std::string key;
    std::vector<std::string> keywords;
};

class Engine_AvatarDefinition
{
public:
    explicit Engine_AvatarDefinition(DefinitionType d_type) : mDefinitionType(d_type) {};

    bool LoadFromFile(const std::string& path);

    DefinitionType GetType() const { return mDefinitionType; }
    const std::vector<BoneDefinition>& GetBoneDefinitions() const { return mBoneDefinitions; }

private:
    DefinitionType mDefinitionType = DefinitionType::None;
    std::vector<BoneDefinition> mBoneDefinitions;
};

class Engine_Humanoid_AvatarDefinition : public Engine_AvatarDefinition
{
    Engine_Humanoid_AvatarDefinition();
};

class Engine_Animal_AvatarDefinition: public Engine_AvatarDefinition
{
    Engine_Animal_AvatarDefinition();
};

class AvatarDefinitionManager
{
public:
    void Initialize(const std::string& definitionFolderPath);

    Engine_AvatarDefinition* GetDefinition(DefinitionType type);

private:
    std::map<DefinitionType, std::shared_ptr<Engine_AvatarDefinition>> mDefinitions;
};