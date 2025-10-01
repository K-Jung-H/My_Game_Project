#include "Scene.h"
#include "GameEngine.h"
#include "Resource/ResourceRegistry.h"

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
	camera_obj->GetComponent<CameraComponent>(Camera)->SetPosition({ 0.0f, 0.0f, 50.0f });
	camera_obj->GetComponent<CameraComponent>(Camera)->SetTarget({ 0.0f, 0.0f, 0.0f });

	auto* resourceManager = GameEngine::Get().GetResourceManager();
	const RendererContext ctx = GameEngine::Get().Get_UploadContext();

	const std::string path = "Assets/CP_100_0012_05/CP_100_0012_05.fbx";
//	const std::string path = "Assets/Scream Tail/pm1086_00_00_lod2.obj";
	Model::loadAndExport(path, "test_assimp_export.txt");
	
	LoadResult result = ResourceRegistry::Instance().Load(*resourceManager, path, "test", ctx);


	auto rcm = GameEngine::Get().GetResourceManager();
	auto model_ptr = rcm->GetById<Model>(result.modelId);
	std::shared_ptr<Object> test_model_obj = Object::Create(model_ptr);
	test_model_obj->GetComponent<TransformComponent>(Transform)->SetScale({ 5, 5, 5 });
	test_model_obj->GetComponent<TransformComponent>(Transform)->SetPosition({0, 0, 0});

	obj_list.push_back(camera_obj);
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
	for (auto obj_ptr : obj_list)
	{
		obj_ptr->Update_Animate(ElapsedTime);
		obj_ptr->Update_Transform_All();
	}
}

void Scene::Render()
{

}

void Scene::RegisterRenderable(std::weak_ptr<MeshRendererComponent> comp)
{
	if (auto c = comp.lock())
	{
		auto it = std::find_if(renderData_list.begin(), renderData_list.end(),
			[&](const RenderData& rd)
			{
				return !rd.meshRenderer.expired() &&
					rd.meshRenderer.lock().get() == c.get();
			});

		if (it == renderData_list.end())
		{
			RenderData rd;
			rd.meshRenderer = comp;

			if (auto owner = c->GetOwner())
			{
				if (auto tf = owner->GetComponent<TransformComponent>(Transform))
					rd.transform = std::static_pointer_cast<TransformComponent>(tf->shared_from_this());
			}

			renderData_list.push_back(rd);
		}
	}
}

std::vector<RenderData> Scene::GetRenderable() const
{
	// Add Camera Culling

	std::vector<RenderData> result;
	result.reserve(renderData_list.size());

	for (const auto& rd : renderData_list)
	{
		if (!rd.meshRenderer.expired() && !rd.transform.expired())
		{
			result.push_back(rd);
		}
	}
	return result;
}
void Scene::RegisterCamera(std::weak_ptr<CameraComponent> cam)
{
	camera_list.push_back(cam);

	if (activeCamera.expired())
		activeCamera = cam;
}
