#include "pch.h"
#include "Scene.h"
#include "GameEngine.h"

Scene::Scene() 
{ 
	scene_id = Engine::INVALID_ID; 
}

Scene::~Scene()
{

}

void Scene::Build()
{
	auto* resourceManager = GameEngine::Get().GetResourceManager();
	const RendererContext ctx = GameEngine::Get().Get_UploadContext();

	const std::string path = "assets/CP_100_0002_63.fbx";
	LoadResult result = ResourceRegistry::Instance().Load(*resourceManager, path, "test", ctx);
}


void Scene::Check_Inputs()
{

}

void Scene::Fixed_Update(float ElapsedTime)
{

}

void Scene::Update(float ElapsedTime)
{

}

void Scene::Render()
{

}