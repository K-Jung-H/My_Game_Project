#pragma once
#include "pch.h"

class SceneManager;

class Scene
{
    friend class SceneManager;

private:
	UINT scene_id;
    std::string alias;

public:
    Scene();
    virtual ~Scene();

protected:
    void SetId(UINT new_id) { scene_id = new_id; }
    void SetAlias(std::string_view a) { alias = a; }

    virtual void Build();

    void Check_Inputs();
    void Fixed_Update(float ElapsedTime); // For Physics
    void Update(float ElapsedTime); // Game Logic
    void Render();

public:
    UINT GetId() const { return scene_id; }
    std::string_view GetAlias() const { return alias; }


};