#include "Scene.h"
#include "GameEngine.h"
#include "Object.h"
#include "Components/RigidbodyComponent.h"

Scene::Scene() 
{ 
	scene_id = Engine::INVALID_ID; 
	m_pObjectManager = std::make_unique<ObjectManager>(this);
}

Scene::~Scene()
{

}

void Scene::Build()
{
	auto rsm = GameEngine::Get().GetResourceSystem();
	const RendererContext ctx = GameEngine::Get().Get_UploadContext();

	//--------------------------------------------------------------------------------
	Object* camera_obj = m_pObjectManager->CreateObject("Main_Camera");
	auto camera_component = camera_obj->AddComponent<CameraComponent>();
	camera_component->SetTransform(camera_obj->GetTransform());
	camera_component->SetPosition({ 0.0f, 10.0f, -30.0f });
	SetActiveCamera(camera_component);

	//--------------------------------------------------------------------------------
	{
		Object* light_obj = m_pObjectManager->CreateObject("Main_Light");
		light_obj->GetTransform()->SetPosition({ 0.0f, 30.0f, 0.0f });

		auto light_component = light_obj->AddComponent<LightComponent>();
		light_component->SetTransform(light_obj->GetTransform());
		light_component->SetLightType(Light_Type::Spot);
	}
	{
		//Object* sub_light_obj = m_pObjectManager->CreateObject("Sub_Light");
		//auto light_component = sub_light_obj->AddComponent<LightComponent>();
		//light_component->SetTransform(sub_light_obj->GetTransform());
	}
	{
		std::shared_ptr<Plane_Mesh> plane_mesh = std::make_shared<Plane_Mesh>(1000.0f, 1000.0f);
		plane_mesh->SetAlias("Plane_Mesh");
		rsm->RegisterResource(plane_mesh);
		UINT plane_id = plane_mesh->GetId();

		Object* plane_obj = m_pObjectManager->CreateObject("Plane_Object");
		plane_obj->GetTransform()->SetPosition({ 0.0f, 0.0f, 0.0f });
		auto mesh_component = plane_obj->AddComponent<MeshRendererComponent>();
		mesh_component->SetMesh(plane_id);
	}
	
	//--------------------------------------------------------------------------------
	
	const std::string path_0 = "Assets/CP_100_0012_05/CP_100_0012_05.fbx";
	const std::string path_1 = "Assets/CP_100_0012_07/CP_100_0012_07.fbx";
	//	const std::string path = "Assets/Scream Tail/pm1086_00_00_lod2.obj";


	{
		LoadResult result;
		rsm->Load(path_0, "Test_0", result);

		auto model_ptr = rsm->GetById<Model>(result.modelId);
		if (!model_ptr)
		{
			OutputDebugStringA("[Scene::Build] Model load failed.\n");
			return;
		}


		Object* test_obj = m_pObjectManager->CreateFromModel(model_ptr);
		m_pObjectManager->SetObjectName(test_obj, "Test_Object_0");
		test_obj->GetTransform()->SetScale({ 5, 5, 5 });
		test_obj->GetTransform()->SetPosition({ 0, 0, 0 });

		auto rb = test_obj->AddComponent<RigidbodyComponent>();
		rb->SetUseGravity(false);
	}

	{
		LoadResult result;
		rsm->Load(path_1, "Test_1", result);

		auto model_ptr = rsm->GetById<Model>(result.modelId);
		for (int i = 0; i < 3; ++i)
		{
			Object* test_obj = m_pObjectManager->CreateFromModel(model_ptr);
			m_pObjectManager->SetObjectName(test_obj, "Test_Object_" + std::to_string(1 + i));
			test_obj->GetTransform()->SetScale({ 5, 5, 5 });
			test_obj->GetTransform()->SetPosition({ 10.0f* (i+1), 0, 0 });
			auto rb = test_obj->AddComponent<RigidbodyComponent>();
			rb->SetUseGravity(false);

		}
	}


//	// For Debug
	bool is_debugging = false;
	if (is_debugging)
	{
		std::shared_ptr<Model> model_ptr;
		Object* test_obj;
		UINT model_node_num = Model::CountNodes(model_ptr);
		UINT node_num = Object::CountNodes(test_obj);
		Model::loadAndExport(path_0, "test_assimp_export.txt");
		Object::DumpHierarchy(test_obj, "test_model_tree.txt");
	}
}


void Scene::Update_Inputs(float dt)
{
	if (auto cam = activeCamera.lock())
	{
		float moveSpeed = 50.0f * dt;
		float rotateSpeed = 0.001f;

		if (InputManager::Get().IsKeyDown(VK_RBUTTON))
		{
			POINT delta = InputManager::Get().GetMouseDelta();

			float yaw = delta.x * rotateSpeed;
			float pitch = delta.y * rotateSpeed;

			cam->AddRotation(-pitch, -yaw, 0.0f);
		}

		XMFLOAT3 pos = cam->GetPosition();
		float speed = 50.0f * dt;

		bool moved = false;

		if (auto tf = cam->GetTransform())
		{
			XMVECTOR q = XMLoadFloat4(&tf->GetRotationQuaternion());

			XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), XMMatrixRotationQuaternion(q));
			XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), XMMatrixRotationQuaternion(q));
			XMVECTOR look = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), XMMatrixRotationQuaternion(q));

			XMVECTOR move = XMVectorZero();

			if (InputManager::Get().IsKeyDown('W')) { move = XMVectorAdd(move, look); moved = true; }
			if (InputManager::Get().IsKeyDown('S')) { move = XMVectorSubtract(move, look); moved = true; }
			if (InputManager::Get().IsKeyDown('A')) { move = XMVectorSubtract(move, right); moved = true; }
			if (InputManager::Get().IsKeyDown('D')) { move = XMVectorAdd(move, right); moved = true; }
			if (InputManager::Get().IsKeyDown('Q')) { move = XMVectorSubtract(move, up); moved = true; }
			if (InputManager::Get().IsKeyDown('E')) { move = XMVectorAdd(move, up); moved = true; }

			if (moved)
			{
				move = XMVector3Normalize(move);
				XMVECTOR posV = XMLoadFloat3(&pos);
				posV = XMVectorAdd(posV, XMVectorScale(move, speed));
				XMStoreFloat3(&pos, posV);

				cam->SetPosition(pos);
			}
		}
	}

	if (InputManager::Get().IsKeyDown('T'))
	{
		auto obj = m_pObjectManager->FindObject("Test_Object_1");
		if(obj)
			m_pObjectManager->DestroyObject(obj->GetId());
	}
}

void Scene::Update_Fixed(float dt) 
{
	GameEngine::Get().GetPhysicsSystem()->Update(scene_id, dt);
}

void Scene::Update_Scene(float dt)
{
	m_pObjectManager->Update_Animate_All(dt);
}

void Scene::Update_Late()
{
	for (auto camera_ptr : camera_list)
	{
		if (auto cp = camera_ptr.lock())
			cp->Update();
	}
	
//	m_pObjectManager->UpdateTransform_All();

	for (auto lightComponent : light_list)
	{
		if (auto lc = lightComponent.lock())
			lc->Update();
	}


	m_pObjectManager->UpdateTransform_All();

}

void Scene::OnComponentRegistered(std::shared_ptr<Component> comp)
{
	if (!comp) return;


	switch (comp->GetType())
	{
	case Component_Type::Mesh_Renderer:
	{
		if (auto mr = std::dynamic_pointer_cast<MeshRendererComponent>(comp))
		{
			RegisterRenderable(mr);
		}
	}
	break;

	case Component_Type::Camera:
	{
		if (auto cam = std::dynamic_pointer_cast<CameraComponent>(comp))
		{
			RegisterCamera(cam);
		}
	}
	break;

	case Component_Type::Light:
	{
		if (auto light = std::dynamic_pointer_cast<LightComponent>(comp))
		{
			light_list.push_back(light);
		}
		break;
	}

	case Component_Type::Rigidbody:
	case Component_Type::Collider:
	{
		if (Object* owner = comp->GetOwner())
			GameEngine::Get().GetPhysicsSystem()->Register(scene_id, owner);
	}
	break;

	default:
		break;
	}
}

void Scene::UnregisterAllComponents(Object* pObject) 
{
	if (!pObject) return;

	auto& componentMap = pObject->GetAllComponents();
	for (auto const& [type, compVec] : componentMap)
	{
		for (const auto& comp : compVec)
		{
			switch (type)
			{
			case Component_Type::Mesh_Renderer:
			{
				auto it = std::remove_if(renderData_list.begin(), renderData_list.end(),
					[&](const RenderData& rd) {return !rd.meshRenderer.expired() && rd.meshRenderer.lock() == comp; });
				renderData_list.erase(it, renderData_list.end());
				break;
			}
			case Component_Type::Light:
			{
				auto it = std::remove_if(light_list.begin(), light_list.end(),
					[&](const std::weak_ptr<LightComponent>& light) {return !light.expired() && light.lock() == comp; });
				light_list.erase(it, light_list.end());
				break;
			}
			case Component_Type::Camera:
			{
				auto it = std::remove_if(camera_list.begin(), camera_list.end(),
					[&](const std::weak_ptr<CameraComponent>& cam) {return !cam.expired() && cam.lock() == comp; });
				camera_list.erase(it, camera_list.end());
				break;
			}
			default:
				break;
			}
		}
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

std::vector<Object*> Scene::GetRootObjectList() const
{
	return m_pObjectManager->GetRootObjects(); 
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

std::vector<LightComponent*> Scene::GetLightList() const
{
	std::vector<LightComponent*> list;
	for (const auto& light_comp : light_list)
	{
		if (auto light = light_comp.lock())
		{ 
			list.push_back(light.get());
		}
	}
	return list;
}


void Scene::RegisterCamera(std::weak_ptr<CameraComponent> cam)
{
	camera_list.push_back(cam);

	if (activeCamera.expired())
		activeCamera = cam;
}
