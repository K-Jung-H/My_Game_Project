#include "ComponentFactory.h"
#include "Core/Object.h"

#include "Components/TransformComponent.h"
#include "Components/MeshRendererComponent.h"
#include "Components/CameraComponent.h"
#include "Components/RigidbodyComponent.h"
#include "Components/ColliderComponent.h"
#include "Components/LightComponent.h"
#include "Components/AnimationControllerComponent.h"
#include "Components/SkinnedMeshRendererComponent.h"


ComponentFactory& ComponentFactory::Instance()
{
    static ComponentFactory instance;
    return instance;
}

ComponentFactory::ComponentFactory()
{
	Initialize();
}

void ComponentFactory::Initialize()
{
    // Object 생성 시 Transform 추가됨,  Add -> Get
    Register(Component_Type::Transform, "TransformComponent",
        [](Object* owner) {
            return owner->GetComponent<TransformComponent>();
        });

    Register(Component_Type::Mesh_Renderer, "MeshRendererComponent",
        [](Object* owner) {
            return owner->AddComponent<MeshRendererComponent>();
        });

    Register(Component_Type::Camera, "CameraComponent",
        [](Object* owner) {
            return owner->AddComponent<CameraComponent>();
        });

    Register(Component_Type::Rigidbody, "RigidbodyComponent",
        [](Object* owner) {
            return owner->AddComponent<RigidbodyComponent>();
        });

    Register(Component_Type::Collider, "ColliderComponent",
        [](Object* owner) {
            return owner->AddComponent<ColliderComponent>();
        });

    Register(Component_Type::Light, "LightComponent",
        [](Object* owner) {
            return owner->AddComponent<LightComponent>();
        });

    Register(Component_Type::AnimationController, "AnimationControllerComponent",
        [](Object* owner) {
            return owner->AddComponent<AnimationControllerComponent>();
        });

    Register(Component_Type::Skinned_Mesh_Renderer, "SkinnedMeshRendererComponent",
        [](Object* owner) {
            return owner->AddComponent<SkinnedMeshRendererComponent>();
        });
}

void ComponentFactory::Register(Component_Type type, const std::string& name, CreatorFunc creator)
{
    mCreators[type] = creator;
    mTypeMap[name] = type;
}

std::shared_ptr<Component> ComponentFactory::Create(Component_Type type, Object* owner)
{
    auto it = mCreators.find(type);
    if (it != mCreators.end())
    {
        return it->second(owner);
    }
    return nullptr;
}

std::shared_ptr<Component> ComponentFactory::Create(const std::string& typeName, Object* owner)
{
    auto it = mTypeMap.find(typeName);
    if (it != mTypeMap.end())
    {
        return Create(it->second, owner);
    }

    return nullptr;
}