#pragma once
#include "Core/Object.h"
#include "Resource/Model.h"

class ObjectManager
{
public:
    ObjectManager() = default;
    ~ObjectManager();

    std::shared_ptr<Object> CreateObject(const std::string& name);
    std::shared_ptr<Object> CreateFromModel(const std::shared_ptr<Model>& model);


    std::shared_ptr<Object> GetById(UINT id);
    std::shared_ptr<Object> FindByName(const std::string& name);


    void DestroyObject(UINT id);
    void Clear();

    UINT AllocateId();
    void ReleaseId(UINT id);

private:
    UINT nextID = 1;
    std::queue<UINT> freeList;

    std::unordered_map<UINT, std::shared_ptr<Object>> activeObjects;
    std::unordered_map<std::string, std::weak_ptr<Object>> nameMap;
};