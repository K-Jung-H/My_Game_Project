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

    void AutoMap(std::shared_ptr<Skeleton> skeleton);

	// void SetMapping(const std::string& key, const std::string& value); // 수동매핑 가능하게 할 때 필요

    void SetDefinitionType(DefinitionType type);
    DefinitionType GetDefinitionType() const { return mDefinitionType; }

    void SetCorrection(const std::string& abstractKey, const XMFLOAT4& rotation);
    XMFLOAT4 GetCorrection(const std::string& abstractKey) const;
    
    const std::string& GetMappedBoneName(const std::string& abstractKey) const;
	const std::map<std::string, std::string>& GetBoneMap() const { return mBoneMap; }

    const std::string& GetMappedKeyByBoneName(const std::string& boneName) const;

private:
    void BuildReverseMap() const;

    DefinitionType mDefinitionType = DefinitionType::None;
    std::map<std::string, std::string> mBoneMap;

    mutable std::unordered_map<std::string, std::string> mReverseBoneMap;
    mutable bool mIsReverseMapDirty = true;

    std::unordered_map<std::string, DirectX::XMFLOAT4> mTPoseCorrections;

};

namespace
{
    std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return (char)std::tolower(c); }
        );
        return s;
    }
}