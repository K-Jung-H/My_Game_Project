#include "SceneArchive.h"
#include "Core/Scene.h"
#include "Core/Object.h"
#include "GameEngine.h"

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
    using namespace rapidjson;

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("scene_id", scene->GetId(), alloc);
    doc.AddMember("alias", Value(scene->GetAlias().c_str(), alloc), alloc);

    Value objs(kArrayType);
    for (auto& pRootObj : scene->GetRootObjectList())
    {
        if (pRootObj)
            objs.PushBack(pRootObj->ToJSON(alloc), alloc);
    }
    doc.AddMember("objects", objs, alloc);

    StringBuffer buf;
    PrettyWriter<StringBuffer> writer(buf);
    doc.Accept(writer);

    std::ofstream ofs(file_name, std::ios::trunc);
    if (!ofs.is_open())
        return false;
    ofs << buf.GetString();
    ofs.close();

    OutputDebugStringA(("[SceneArchive] Saved scene: " + file_name + "\n").c_str());
    return true;
}

Object* SceneArchive::LoadObjectRecursive(Scene* scene, const rapidjson::Value& val)
{
    auto om = scene->GetObjectManager();

    std::string name = val["name"].GetString();
    UINT id = val["id"].GetUint();

    Object* obj = om->CreateObjectWithId(name, id);
    if (!obj) return nullptr;

    obj->FromJSON(val);

    if (val.HasMember("children"))
    {
        for (auto& childVal : val["children"].GetArray())
        {
            Object* childObj = LoadObjectRecursive(scene, childVal);
            if (childObj) {
                om->SetParent(childObj, obj);
            }
        }
    }

    return obj;
}

std::shared_ptr<Scene> SceneArchive::LoadJSON(const std::string& file_name)
{
    using namespace rapidjson;

    std::ifstream ifs(file_name);
    if (!ifs.is_open())
    {
        OutputDebugStringA(("[SceneArchive] Cannot open file: " + file_name + "\n").c_str());
        return nullptr;
    }

    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    Document doc;
    doc.Parse(json.c_str());

    if (doc.HasParseError())
    {
        OutputDebugStringA("[SceneArchive] JSON Parse Error\n");
        return nullptr;
    }

    auto scene = std::make_shared<Scene>();
    scene->SetId(doc["scene_id"].GetUint());
    scene->SetAlias(doc["alias"].GetString());

    if (doc.HasMember("objects") && doc["objects"].IsArray())
    {
        for (auto& rootVal : doc["objects"].GetArray())
        {
            LoadObjectRecursive(scene.get(), rootVal);
        }
    }

    if (!doc.HasMember("objects") || !doc["objects"].IsArray())
    {
        OutputDebugStringA("[SceneArchive] No objects found in scene file.\n");
        return scene;
    }

    OutputDebugStringA("[SceneArchive] Scene Load Completed.\n");
    return scene;
}




bool SceneArchive::SaveBinary(const std::shared_ptr<Scene>& scene, const std::string& file_name)
{
    return false;
}


std::shared_ptr<Scene> SceneArchive::LoadBinary(const std::string& file_name)
{
    return nullptr;
}
