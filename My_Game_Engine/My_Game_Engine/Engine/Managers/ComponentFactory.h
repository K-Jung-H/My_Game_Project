#pragma once
#include <functional>
#include "Core/Component.h" 

class Object; 

class ComponentFactory
{
public:
    using CreatorFunc = std::function<std::shared_ptr<Component>(Object*)>;

    static ComponentFactory& Instance();


    std::shared_ptr<Component> Create(Component_Type type, Object* owner);
    std::shared_ptr<Component> Create(const std::string& typeName, Object* owner);

private:
    ComponentFactory();
    ~ComponentFactory() = default;

    void Initialize();

    void Register(Component_Type type, const std::string& name, CreatorFunc creator);

private:
    std::unordered_map<Component_Type, CreatorFunc> mCreators;
    std::unordered_map<std::string, Component_Type> mTypeMap;
};