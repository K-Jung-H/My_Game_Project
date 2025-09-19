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
	std::shared_ptr<Object> camera_obj = Object::Create("Main_Camera");
	camera_obj->AddComponent<CameraComponent>();


	auto* resourceManager = GameEngine::Get().GetResourceManager();
	const RendererContext ctx = GameEngine::Get().Get_UploadContext();

	const std::string path = "assets/CP_100_0002_63.fbx";
	LoadResult result = ResourceRegistry::Instance().Load(*resourceManager, path, "test", ctx);

	std::shared_ptr<Object> test_obj = Object::Create("test_obj");

	auto rcm = GameEngine::Get().GetResourceManager();
	auto model_ptr = rcm->GetById<Model>(result.modelId);
	std::shared_ptr<Object> test_model_obj = Object::Create(model_ptr);


	obj_list.push_back(camera_obj);
	obj_list.push_back(test_obj);
	obj_list.push_back(test_model_obj);

	// For Debug
	UINT model_node_num = Model::CountNodes(model_ptr);
	UINT node_num = Object::CountNodes(test_model_obj);
	Object::DumpHierarchy(test_model_obj, "test_model_tree.txt");
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

void Scene::RegisterRenderable(std::weak_ptr<MeshRendererComponent> comp)
{
	if (auto c = comp.lock())
	{
		auto it = std::find_if(renderable_list.begin(), renderable_list.end(),
			[&](const std::weak_ptr<Component>& w)
			{
				return !w.expired() && w.lock().get() == c.get();
			});

		if (it == renderable_list.end())
			renderable_list.push_back(comp);
	}
}

std::vector<std::shared_ptr<MeshRendererComponent>> Scene::GetRenderable() const
{
	// Add Camera Culling

	std::vector<std::shared_ptr<MeshRendererComponent>> result;
	result.reserve(renderable_list.size());

	for (auto& w : renderable_list) 
	{
		if (auto s = w.lock())
			result.push_back(s);
	}
	return result;
}

void Scene::RegisterCamera(std::weak_ptr<CameraComponent> cam)
{
	camera_list.push_back(cam);

	if (activeCamera.expired())
		activeCamera = cam;
}
