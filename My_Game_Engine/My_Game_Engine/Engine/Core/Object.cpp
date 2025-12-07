#include "Object.h"
#include "GameEngine.h"
#include "Managers/ComponentFactory.h"
#include "Components/TransformComponent.h"
#include "Components/MeshRendererComponent.h"

Object::Object(const std::string& name) : mName(name)
{
    transform = std::make_shared<TransformComponent>();
    transform->SetOwner(this);
    map_Components[TransformComponent::Type].push_back(transform);
}

Object::~Object()
{

}

void Object::WakeUpRecursive()
{
    for (auto& [type, compVec] : map_Components)
    {
        for (auto& c : compVec)
        {
            if (c)
                c->WakeUp();
        }
    }

    for (auto& child : m_pChildren)
    {
        if (child)
            child->WakeUpRecursive();
    }
}

void Object::SetParent(Object* new_parent)
{
    if (m_pObjectManager)
        m_pObjectManager->SetParent(this, new_parent);
    else
        throw std::logic_error("Object's ObjectManager is NULL");
}

void Object::SetChild(Object* new_child)
{
    if (m_pObjectManager)
        m_pObjectManager->SetChild(this, new_child);
    else
        throw std::logic_error("Object's ObjectManager is NULL");
}

void Object::SetSibling(Object* new_sibling)
{
    if (m_pObjectManager)
        m_pObjectManager->SetSibling(this, new_sibling);
    else
        throw std::logic_error("Object's ObjectManager is NULL");
}

void Object::Update_Animate(float dt)
{
}

void Object::UpdateTransform_All()
{
    XMFLOAT4X4 identity;
    XMStoreFloat4x4(&identity, XMMatrixIdentity());

    Update_Transform(&identity, true);
}

void Object::Update_Transform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty)
{
    const XMFLOAT4X4* worldForChildren = parentWorld;
    bool myWorldDirty = parentWorldDirty;

    auto transform = GetTransform();

    if (transform)
    {
        myWorldDirty = transform->UpdateTransform(parentWorld, parentWorldDirty);
        worldForChildren = &transform->GetWorldMatrix();
    }

    for (auto& child : m_pChildren)
    {
        if (!child) continue;
        child->Update_Transform(worldForChildren, myWorldDirty);
    }
}

rapidjson::Value Object::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value val(kObjectType);

    val.AddMember("id", object_ID, alloc);
    val.AddMember("name", Value(mName.c_str(), alloc), alloc);

    if (transform)
        val.AddMember("transform", transform->ToJSON(alloc), alloc);

    Value comps(kArrayType);
    for (auto& [type, compVec] : map_Components)
    {
        for (auto& c : compVec)
            comps.PushBack(c->ToJSON(alloc), alloc);
    }
    val.AddMember("components", comps, alloc);

    Value childrenArr(kArrayType);
    for (auto& ch : m_pChildren)
    {
        if (ch)
            childrenArr.PushBack(ch->ToJSON(alloc), alloc);
    }

    val.AddMember("children", childrenArr, alloc);

    return val;
}

void Object::FromJSON(const rapidjson::Value& val)
{
    mName = val["name"].GetString();
    object_ID = val["id"].GetUint();

    if (val.HasMember("transform") && val["transform"].IsObject())
        transform->FromJSON(val["transform"]);

    if (val.HasMember("components"))
    {
        for (auto& compVal : val["components"].GetArray())
        {
            if (!compVal.HasMember("type")) continue;
            std::string typeStr = compVal["type"].GetString();

            auto newComp = ComponentFactory::Instance().Create(typeStr, this);

            if (newComp)
                newComp->FromJSON(compVal);
        }
    }
}

UINT Object::CountNodes(Object* root)
{
    std::unordered_set<Object*> visited;

    std::function<UINT(Object*)> dfs;
    dfs = [&](Object* obj) -> UINT
        {
            if (!obj) return 0;
            if (visited.count(obj)) return 0;
            visited.insert(obj);

            UINT count = 1;
            for (auto& child : obj->GetChildren())
            {
                if (child)
                    count += dfs(child);
            }
            return count;
        };

    return dfs(root);
}

void Object::DumpHierarchy(Object* root, const std::string& filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open())
        return;

    std::function<void(Object*, int)> recurse;
    recurse = [&](Object* obj, int depth)
        {
            if (!obj) return;

            for (int i = 0; i < depth; ++i) ofs << "\t";

            ofs << "- Object: " << obj->GetId() << " (" << obj->GetName() << ")";

            auto renderers = obj->GetComponents<MeshRendererComponent>();
            if (!renderers.empty())
            {
                ofs << " [MeshRenderers: " << renderers.size() << "]";

                ofs << " [SubMeshes:";
                bool first = true;
                for (auto& mr : renderers)
                {
                    if (auto mesh = mr->GetMesh())
                    {
                        if (!first) ofs << ",";
                        ofs << " " << mesh->submeshes.size();
                        first = false;
                    }
                }
                ofs << "]";
            }

            ofs << "\n";

            for (auto Child : obj->GetChildren())
            {
                if (Child)
                    recurse(Child, depth + 1);
            }
        };

    recurse(root, 0);
}