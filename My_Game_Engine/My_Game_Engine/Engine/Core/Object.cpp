#include "pch.h"
#include "Object.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/MeshRendererComponent.h"
#include "../Components/ColliderComponent.h"


std::shared_ptr<Object> Object::Create(const std::string& name)
{
    auto obj = std::shared_ptr<Object>(new Object(name));
    obj->AddComponent<TransformComponent>();
    return obj;
}


Object::~Object()
{

}

void Object::Update(float dt) 
{
    for (auto& c : map_Components)
        c.second->Update(dt);
}

void Object::Render() 
{
    for (auto& c : map_Components)
        c.second->Render();
}
