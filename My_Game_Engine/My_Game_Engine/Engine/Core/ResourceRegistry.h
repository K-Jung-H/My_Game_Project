#pragma once
#include "Resource.h"
#include "ResourceManager.h"
class ResourceRegistry
{
public:
    static ResourceRegistry& Instance()
    {
        static ResourceRegistry instance;
        return instance;
    }

    // 복사/이동 금지
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;

    template<typename T>
    std::shared_ptr<T> Load(ResourceManager& manager,
        const std::string& path,
        std::string_view alias = "");

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;

    UINT mNextResourceID = 1;
};