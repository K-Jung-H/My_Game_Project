#include "Scene.h"
#include "GameEngine.h"
#include "Resource/ResourceRegistry.h"
#include "Components/RigidbodyComponent.h"

Scene::Scene() 
{ 
	scene_id = Engine::INVALID_ID; 
}

Scene::~Scene()
{

}

void Scene::Build()
{
	auto om = GameEngine::Get().GetObjectManager();
	auto* rcm = GameEngine::Get().GetResourceManager();
	const RendererContext ctx = GameEngine::Get().Get_UploadContext();

	//--------------------------------------------------------------------------------
	std::shared_ptr<Object> camera_obj = om->CreateObject("Main_Camera");
	auto camera_component = camera_obj->AddComponent<CameraComponent>();
	camera_component->SetPosition({ 0.0f, 0.0f, 50.0f });
	camera_component->SetTarget({ 0.0f, 0.0f, 0.0f });
	SetActiveCamera(camera_component);
	obj_root_list.push_back(camera_obj);

	//--------------------------------------------------------------------------------

	const std::string path = "Assets/CP_100_0012_05/CP_100_0012_05.fbx";
//	const std::string path = "Assets/Scream Tail/pm1086_00_00_lod2.obj";
	Model::loadAndExport(path, "test_assimp_export.txt");
	
	LoadResult result = ResourceRegistry::Instance().Load(*rcm, path, "test", ctx);
	auto model_ptr = rcm->GetById<Model>(result.modelId);


	std::shared_ptr<Object> test_obj = om->CreateFromModel(model_ptr);
	test_obj->GetTransform()->SetScale({5, 5, 5});
	test_obj->GetTransform()->SetPosition({0, 0, 0});
	auto rb = test_obj->AddComponent<RigidbodyComponent>();
	rb->SetUseGravity(false);


	obj_root_list.push_back(test_obj);

	// For Debug
	UINT model_node_num = Model::CountNodes(model_ptr);
	UINT node_num = Object::CountNodes(test_obj);
	Object::DumpHierarchy(test_obj, "test_model_tree.txt");
}


void Scene::Update_Inputs(float dt)
{
	if (auto cam = activeCamera.lock())
	{
		XMFLOAT3 pos = cam->GetPosition();
		float speed = 50.0f * dt;

		if (InputManager::Get().IsKeyDown('W')) pos.z += speed;
		if (InputManager::Get().IsKeyDown('S')) pos.z -= speed;
		if (InputManager::Get().IsKeyDown('A')) pos.x -= speed;
		if (InputManager::Get().IsKeyDown('D')) pos.x += speed;
		if (InputManager::Get().IsKeyDown('Q')) pos.y -= speed;
		if (InputManager::Get().IsKeyDown('E')) pos.y += speed;

		cam->SetPosition(pos);
	}
}

void Scene::Update_Fixed(float dt) 
{
	GameEngine::Get().GetPhysicsSystem()->Update(scene_id, dt);
}

void Scene::Update_Scene(float dt)
{
	for (auto obj_ptr : obj_root_list)
	{
		obj_ptr->Update_Animate(dt);
	}
}

void Scene::Update_Late(float dt)
{
	for (auto obj_ptr : obj_root_list)
	{
		obj_ptr->UpdateTransform_All();
	}
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
				if (auto tf = owner->GetTransform())
					rd.transform = tf;
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
