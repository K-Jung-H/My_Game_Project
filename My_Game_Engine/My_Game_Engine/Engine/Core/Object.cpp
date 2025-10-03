#include "Object.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/MeshRendererComponent.h"
#include "../Components/ColliderComponent.h"
#include "../Resource/Model.h"

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
                for (auto* mr : renderers)
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



static std::shared_ptr<Object> ConvertNode(const std::shared_ptr<Model>& model, const std::shared_ptr<Model::Node>& node, std::shared_ptr<Object> parent)
{
    const Skeleton& skeleton = model->GetSkeleton();

    auto obj = Object::Create(node->name);
    if (parent)
        obj->SetParent(parent);

    if (auto transform = obj->GetComponent<TransformComponent>(Transform))
        transform->SetFromMatrix(node->localTransform);

    for (auto& mesh : node->meshes) 
    {
        if (!mesh) continue;
        auto& mr = obj->AddComponent<MeshRendererComponent>();
        mr.SetMesh(mesh->GetId());
    }

    for (auto& child : node->children) 
        ConvertNode(model, child, obj);


    return obj;
}

static std::shared_ptr<Object> ConvertModelToObjects(const std::shared_ptr<Model>& model)
{
    if (!model || !model->GetRoot())
        return nullptr;

    return ConvertNode(model, model->GetRoot(), nullptr);
}

std::shared_ptr<Object> Object::Create(const std::shared_ptr<Model> model)
{
    return ConvertModelToObjects(model);
}

std::shared_ptr<Object> Object::Create(const std::string& name)
{
    auto obj = std::shared_ptr<Object>(new Object(name));
    obj->AddComponent<TransformComponent>();
    return obj;
}

Object::~Object()
{

}

void Object::SetParent(std::shared_ptr<Object> new_parent)
{
    parent = new_parent;
    if (new_parent)
        new_parent->children.push_back(shared_from_this());
}

void Object::SetChild(std::shared_ptr<Object> new_child)
{
    if (!new_child) return;
    new_child->parent = shared_from_this();
    children.push_back(new_child);
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

void Object::UpdateMotion_All(float dt)
{
    auto transform = GetComponent<TransformComponent>(Transform);
    if (transform)
        transform->UpdateMotion(dt); 

    for (auto& child : children)
    {
        if (child) child->UpdateMotion_All(dt);
    }
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

    auto transform = GetComponent<TransformComponent>(Transform);

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

