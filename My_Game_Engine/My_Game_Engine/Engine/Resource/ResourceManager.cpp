#pragma once
#include "ResourceManager.h"
#include "MetaIO.h"

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

    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".dae")
        return FileCategory::ComplexModel;

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".tga" || ext == ".bmp" || ext == ".dds" || ext == ".hdr")
        return FileCategory::Texture;

    if (ext == ".mat")
        return FileCategory::Material;

    return FileCategory::Unknown;
}

void ResourceManager::Add(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return;

    if (MetaIO::LoadSimpleMeta(res) == false)
    {

        MetaIO::EnsureResourceGUID(res);

        std::string pathCopy = std::string(res->GetPath());
        FileCategory category = DetectFileCategory(pathCopy);

        switch (category)
        {
        case FileCategory::ComplexModel:
            break;

        case FileCategory::Material:
            MetaIO::SaveSimpleMeta(res);
            break;

        case FileCategory::Texture:
            MetaIO::SaveSimpleMeta(res);
            break;

        default:
            MetaIO::SaveSimpleMeta(res);
            break;
        }
    }

    ResourceEntry entry;
    entry.id = res->GetId();
    entry.path = res->GetPath();
    entry.alias = std::string(res->GetAlias());
    entry.resource = res;

    mGuidMap[res->GetGUID()] = res;

    mPathToId[entry.path] = entry.id;
    mAliasToId[entry.alias] = entry.id;
    map_Resources[entry.id] = std::move(entry);

    if (auto mesh = std::dynamic_pointer_cast<Mesh>(res))
    {
        mMeshes.push_back(mesh);
    }
    else if (auto tex = std::dynamic_pointer_cast<Texture>(res))
    {
        mTextures.push_back(tex);
    }
    else if (auto mat = std::dynamic_pointer_cast<Material>(res))
    {
        mMaterials.push_back(mat);
    }
}
