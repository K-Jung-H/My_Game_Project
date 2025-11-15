#pragma once
#include "Resource/Skeleton.h"
#include "AvatarSystem.h" 


class Model_Avatar : public Game_Resource
{
public:
    Model_Avatar();
    virtual ~Model_Avatar() = default;

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;
    virtual bool SaveToFile(const std::string& outputPath = "") const;
    void SetDefinitionType(DefinitionType type);
    void AutoMap(std::shared_ptr<Skeleton> skeleton);

	// void SetMapping(const std::string& key, const std::string& value); // 수동매핑 가능하게 할 때 필요

    DefinitionType GetDefinitionType() const { return mDefinitionType; }
    const std::string& GetMappedBoneName(const std::string& abstractKey) const;

private:
    DefinitionType mDefinitionType = DefinitionType::None;
    std::map<std::string, std::string> mBoneMap;
};