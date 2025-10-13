#include "Object.h"
#include "GameEngine.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/MeshRendererComponent.h"
#include "Components/RigidbodyComponent.h"
#include "Components/ColliderComponent.h"
#include "Resource/Model.h"


rapidjson::Value Object::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    using namespace rapidjson;
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
    for (auto& ch : children)
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
            std::string type = compVal["type"].GetString();
            if (type == "CameraComponent") AddComponent<CameraComponent>()->FromJSON(compVal);
            else if (type == "RigidbodyComponent") AddComponent<RigidbodyComponent>()->FromJSON(compVal);
            else if (type == "MeshRendererComponent") AddComponent<MeshRendererComponent>()->FromJSON(compVal);
        }
    }
}


static std::shared_ptr<Object> ConvertNode(const std::shared_ptr<Scene> scene, const std::shared_ptr<Model>& model, const std::shared_ptr<Model::Node>& node, std::shared_ptr<Object> parent)
{
    auto om = GameEngine::Get().GetObjectManager();
    const Skeleton& skeleton = model->GetSkeleton();

    auto obj = om->CreateObject(scene, node->name);
    if (parent)
        obj->SetParent(parent);

    if (auto transform = obj->GetTransform())
        transform->SetFromMatrix(node->localTransform);

    for (auto& mesh : node->meshes)
    {
        if (!mesh) continue;
        auto mr = obj->AddComponent<MeshRendererComponent>();
        mr->SetMesh(mesh->GetId());
    }

    for (auto& child : node->children)
        ConvertNode(scene, model, child, obj);


    return obj;
}

UINT Object::CountNodes(const std::shared_ptr<Object>& root)
{
    std::unordered_set<const Object*> visited;

    std::function<UINT(const std::shared_ptr<Object>&)> dfs;
    dfs = [&](const std::shared_ptr<Object>& obj) -> UINT
        {
            if (!obj) return 0;
            if (visited.count(obj.get())) return 0;
            visited.insert(obj.get());

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


void Object::DumpHierarchy(const std::shared_ptr<Object>& root, const std::string& filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open())
        return;

    std::function<void(const std::shared_ptr<Object>&, int)> recurse;
    recurse = [&](const std::shared_ptr<Object>& obj, int depth)
        {
            if (!obj) return;

            for (int i = 0; i < depth; ++i) ofs << "\t";

            ofs << "- Object: " << obj->GetId() << " (" << obj->GetName() << ")";

            auto renderers = obj->GetComponents<MeshRendererComponent>(MeshRendererComponent::Type);
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

            for (auto& Child : obj->GetChildren())
            {
                if (Child)
                    recurse(Child, depth + 1);
            }
        };

    recurse(root, 0);
}


static std::shared_ptr<Object> ConvertModelToObjects(const std::shared_ptr<Scene> scene, const std::shared_ptr<Model>& model)
{
    if (!model || !model->GetRoot())
        return nullptr;

    return ConvertNode(scene, model, model->GetRoot(), nullptr);
}

std::shared_ptr<Object> Object::Create(const std::shared_ptr<Scene> scene, const std::shared_ptr<Model> model)
{
    return ConvertModelToObjects(scene, model);
}

std::shared_ptr<Object> Object::Create(const std::shared_ptr<Scene> scene, const std::string& name)
{
    std::shared_ptr<Object> obj(new Object(name)); 
    obj->Initialize(scene);

    return obj;
}

void Object::Initialize(const std::shared_ptr<Scene>& scene)
{
    mScene = scene;
    transform = AddComponent<TransformComponent>();
}

Object::~Object()
{

}

void Object::SetScene(std::weak_ptr<Scene> s)
{
    mScene = s;
    for (auto& child : children)
    {
        if (child)
            child->SetScene(s);
    }
}

void Object::SetParent(std::shared_ptr<Object> new_parent)
{
    parent = new_parent;

    if (new_parent)
    {
        new_parent->children.push_back(shared_from_this());

        if (auto scenePtr = new_parent->GetScene().lock())
            SetScene(scenePtr);

    }
    else
        mScene.reset();
}

void Object::SetChild(std::shared_ptr<Object> new_child)
{
    if (!new_child) return;

    new_child->parent = shared_from_this();
    children.push_back(new_child);

    if (auto scenePtr = GetScene().lock())
        new_child->SetScene(scenePtr);

}

void Object::SetSibling(std::shared_ptr<Object> new_sibling)
{
    if (!new_sibling) return;
    if (auto p = parent.lock())
    {
        new_sibling->parent = p;
        p->children.push_back(new_sibling);
    }
}


std::weak_ptr<Object> Object::GetParent()
{
    return parent;
}

const std::vector<std::shared_ptr<Object>>& Object::GetChildren()
{
    return children;
}

std::vector<std::shared_ptr<Object>> Object::GetSiblings()
{
    std::vector<std::shared_ptr<Object>> result;
    if (auto p = parent.lock())
    {
        for (auto& s : p->children) 
        {
            if (s.get() != this)    
                result.push_back(s); 
        }
    }
    return result;
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

    for (auto& child : children) 
    {
        if (!child) continue;
        child->Update_Transform(worldForChildren, myWorldDirty);
    }
}