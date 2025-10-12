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
	auto rsm = GameEngine::Get().GetResourceSystem();
	const RendererContext ctx = GameEngine::Get().Get_UploadContext();

	//--------------------------------------------------------------------------------
	std::shared_ptr<Object> camera_obj = om->CreateObject(shared_from_this(), "Main_Camera");
	auto camera_component = camera_obj->AddComponent<CameraComponent>();
	camera_component->SetTransform(camera_obj->GetTransform());
	camera_component->SetPosition({ 0.0f, 0.0f, 50.0f });

	SetActiveCamera(camera_component);
	RegisterObject(camera_obj);

	//--------------------------------------------------------------------------------

	const std::string path_0 = "Assets/CP_100_0012_05/CP_100_0012_05.fbx";
	const std::string path_1 = "Assets/CP_100_0012_07/CP_100_0012_07.fbx";
	Model::loadAndExport(path_0, "test_assimp_export.txt");

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


		std::shared_ptr<Object> test_obj = om->CreateFromModel(shared_from_this(), model_ptr);
		test_obj->SetName("Test_Object_0");
		test_obj->GetTransform()->SetScale({ 5, 5, 5 });
		test_obj->GetTransform()->SetPosition({ 0, 0, 0 });

		auto rb = test_obj->AddComponent<RigidbodyComponent>();
		rb->SetUseGravity(false);

		RegisterObject(test_obj);
	}

	{
		LoadResult result;
		rsm->Load(path_1, "Test_1", result);

		auto model_ptr = rsm->GetById<Model>(result.modelId);
		for (int i = 0; i < 3; ++i)
		{
			std::shared_ptr<Object> test_obj = om->CreateFromModel(shared_from_this(), model_ptr);
			test_obj->SetName("Test_Object_" + std::to_string(1+i));
			test_obj->GetTransform()->SetScale({ 5, 5, 5 });
			test_obj->GetTransform()->SetPosition({ 10.0f* (i+1), 0, 0 });
			auto rb = test_obj->AddComponent<RigidbodyComponent>();
			rb->SetUseGravity(false);

			RegisterObject(test_obj);
		}
	}


//	// For Debug
//	UINT model_node_num = Model::CountNodes(model_ptr_1);
//	UINT node_num = Object::CountNodes(test_obj_1);
//	Object::DumpHierarchy(test_obj_1, "test_model_tree.txt");
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
	for (auto camera_ptr : camera_list)
	{
		if (auto cp = camera_ptr.lock())
			cp->Update();
	}

	
	for (auto obj_ptr : obj_root_list)
	{
		obj_ptr->UpdateTransform_All();
	}
}

void Scene::RegisterObject(const std::shared_ptr<Object>& obj)
{
	if (!obj) return;

	if (obj->GetParent().expired())
			obj_root_list.push_back(obj);

	obj_map[obj->GetId()] = obj;

	obj->SetScene(weak_from_this());


	for (auto& [type, comps] : obj->GetComponents())
	{
		for (auto& c : comps)
			RegisterComponent(c);
	}

	for (auto& child : obj->GetChildren())
	{
		if (child)
			RegisterObject(child); 
	}
}


void Scene::RegisterComponent(std::weak_ptr<Component> comp)
{
	if (auto c = comp.lock())
	{
		switch (c->GetType())
		{
		case Component_Type::Mesh_Renderer:
		{
			if (auto mr = std::dynamic_pointer_cast<MeshRendererComponent>(c))
			{
				RegisterRenderable(mr);
			}
		}
		break;

		case Component_Type::Camera:
		{
			if (auto cam = std::dynamic_pointer_cast<CameraComponent>(c))
			{
				RegisterCamera(cam);
			}
		}
		break;

		case Component_Type::Rigidbody:
		case Component_Type::Collider:
		{
			if (auto owner = c->GetOwner())
				GameEngine::Get().GetPhysicsSystem()->Register(scene_id, owner);
		}
		break;

		default:
			break;
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
