#include "MeshRendererComponent.h"
#include "GameEngine.h"


rapidjson::Value MeshRendererComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    v.AddMember("type", "MeshRendererComponent", alloc);

    auto rm = GameEngine::Get().GetResourceManager();
    auto mesh = rm->GetById<Mesh>(meshId);
    if (!mesh)
        return v;

    std::string mesh_guid = mesh->GetGUID();
    v.AddMember("mesh_guid", Value(mesh_guid.c_str(), alloc), alloc);

    Value subArr(kArrayType);
    for (size_t i = 0; i < mesh->submeshes.size(); ++i)
    {
        Value entry(kObjectType);
        entry.AddMember("index", static_cast<uint32_t>(i), alloc);

        UINT matId = (i < materialOverrides.size() && materialOverrides[i] != Engine::INVALID_ID)
            ? materialOverrides[i] : mesh->submeshes[i].materialId;;

        if (auto mat = rm->GetById<Material>(matId))
        {
            std::string mat_guid = mat->GetGUID();
            entry.AddMember("material_guid", Value(mat_guid.c_str(), alloc), alloc);
        }
        else
        {
            entry.AddMember("material_guid", "", alloc);
        }

        subArr.PushBack(entry, alloc);
    }

    v.AddMember("submeshes", subArr, alloc);
    return v;
}

void MeshRendererComponent::FromJSON(const rapidjson::Value& val)
{
    auto rm = GameEngine::Get().GetResourceManager();

    if (!val.HasMember("mesh_guid") || !val["mesh_guid"].IsString())
        return;

    auto mesh = rm->GetByGUID<Mesh>(val["mesh_guid"].GetString());
    if (!mesh)
        return;

    SetMesh(mesh->GetId()); 

    if (!val.HasMember("submeshes") || !val["submeshes"].IsArray())
        return;

    const auto& subs = val["submeshes"].GetArray();
    for (const auto& s : subs)
    {
        if (!s.HasMember("index") || !s.HasMember("material_guid"))
            continue;

        if (!s["index"].IsUint() || !s["material_guid"].IsString())
            continue;

        size_t idx = s["index"].GetUint();
        if (idx >= materialOverrides.size())
            continue;

        auto mat = rm->GetByGUID<Material>(s["material_guid"].GetString());
        if (mat)
            materialOverrides[idx] = mat->GetId();
        else
            materialOverrides[idx] = Engine::INVALID_ID;
    }
}


void MeshRendererComponent::SetMesh(UINT id)
{
    meshId = id;
    auto rc = GameEngine::Get().GetResourceManager();
    
    mMesh = rc->GetById<Mesh>(id);

    if (auto mesh = mMesh.lock())
    {
        materialOverrides.resize(mesh->submeshes.size(), Engine::INVALID_ID);

        for (size_t i = 0; i < mesh->submeshes.size(); ++i)
            materialOverrides[i] = mesh->submeshes[i].materialId;

    }
    else
        materialOverrides.clear();

}

void MeshRendererComponent::SetMaterial(size_t submeshIndex, UINT matId)
{
    if (submeshIndex >= materialOverrides.size()) return;
    materialOverrides[submeshIndex] = matId;
}

UINT MeshRendererComponent::GetMaterial(size_t submeshIndex) const
{
    if (submeshIndex >= materialOverrides.size()) 
        return Engine::INVALID_ID;

    return materialOverrides[submeshIndex];
}

