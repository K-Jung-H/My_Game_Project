#include "MeshRendererComponent.h"
#include "GameEngine.h"


rapidjson::Value MeshRendererComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    v.AddMember("type", "MeshRendererComponent", alloc);

    auto rsm = GameEngine::Get().GetResourceSystem();
    auto mesh = rsm->GetById<Mesh>(meshId);
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

        if (auto mat = rsm->GetById<Material>(matId))
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
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();
    const RendererContext& ctx = GameEngine::Get().Get_UploadContext();

    // --- Mesh 복원 ---
    if (val.HasMember("mesh_guid") && val["mesh_guid"].IsString())
    {
        std::string meshGuid = val["mesh_guid"].GetString();
        auto mesh = rs->GetByGUID<Mesh>(meshGuid);

        // GUID로 못 찾았을 경우, Path 기반 fallback
        if (!mesh && val.HasMember("mesh_path") && val["mesh_path"].IsString())
        {
            std::string meshPath = val["mesh_path"].GetString();
            LoadResult temp;
            rs->Load(meshPath, "LoadedMesh", temp);
            mesh = rs->GetByGUID<Mesh>(meshGuid);
        }

        if (mesh)
        {
            SetMesh(mesh->GetId());
        }
        else
        {
            OutputDebugStringA(("[MeshRenderer] Missing mesh GUID: " + meshGuid + "\n").c_str());
            return;
        }
    }

    // --- Submesh별 Material 복원 ---
    if (val.HasMember("submeshes") && val["submeshes"].IsArray())
    {
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

            std::string matGuid = s["material_guid"].GetString();
            auto mat = rs->GetByGUID<Material>(matGuid);

            // fallback: GUID로 못 찾으면 Path 기반 재로드
            if (!mat && s.HasMember("material_path") && s["material_path"].IsString())
            {
                std::string matPath = s["material_path"].GetString();
                LoadResult temp;
                rs->Load(matPath, "LoadedMat", temp);
                mat = rs->GetByGUID<Material>(matGuid);
            }

            if (mat)
                materialOverrides[idx] = mat->GetId();
            else
            {
                OutputDebugStringA(("[MeshRenderer] Missing material GUID: " + matGuid + "\n").c_str());
                materialOverrides[idx] = Engine::INVALID_ID;
            }
        }
    }
}

void MeshRendererComponent::SetMesh(UINT id)
{
    auto rsm = GameEngine::Get().GetResourceSystem();

    meshId = id;

    mMesh = rsm->GetById<Mesh>(id);

    if (auto mesh = mMesh.lock())
    {
        materialOverrides.resize(mesh->submeshes.size(), Engine::INVALID_ID);
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

