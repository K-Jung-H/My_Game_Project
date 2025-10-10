#include "SceneArchive.h"
#include "Core/Scene.h"


using namespace rapidjson;

bool SceneArchive::Save(const std::shared_ptr<Scene>& scene, const std::string& file_name, SceneFileFormat format)
{
    switch (format)
    {
    case SceneFileFormat::JSON:   return SaveJSON(scene, file_name);
    case SceneFileFormat::Binary: return SaveBinary(scene, file_name);
    default: return false;
    }
}

std::shared_ptr<Scene> SceneArchive::Load(const std::string& file_name, SceneFileFormat format)
{
    switch (format)
    {
    case SceneFileFormat::JSON:   return LoadJSON(file_name);
    case SceneFileFormat::Binary: return LoadBinary(file_name);
    default: return nullptr;
    }
}


bool SceneArchive::SaveJSON(const std::shared_ptr<Scene>& scene, const std::string& file_name)
{
    return false;
}


std::shared_ptr<Scene> SceneArchive::LoadJSON(const std::string& file_name)
{
    return nullptr;
}


bool SceneArchive::SaveBinary(const std::shared_ptr<Scene>& scene, const std::string& file_name)
{
    return false;
}


std::shared_ptr<Scene> SceneArchive::LoadBinary(const std::string& file_name)
{
    return nullptr;
}
