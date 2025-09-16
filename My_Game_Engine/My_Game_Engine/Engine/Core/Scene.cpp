#include "pch.h"
#include "Scene.h"
#include "GameEngine.h"
#include "../Resource/ResourceRegistry.h"
#include "../Resource/Model.h"

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

	std::shared_ptr<Object> test_obj = Object::Create("test_obj");

	auto rcm = GameEngine::Get().GetResourceManager();
	auto model_ptr = rcm->GetById<Model>(result.modelId);
	std::shared_ptr<Object> test_model_obj = Object::Create(model_ptr);

	Object::DumpHierarchy(test_model_obj, "test_model_tree.txt");
	UINT node_num = Object::CountNodes(test_model_obj);

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