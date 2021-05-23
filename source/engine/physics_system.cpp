#include "physics_system.h"
#include "system_enumerator.h"
#include "debug_gui_system.h"
#include "job_system.h"
#include "components/component_physics.h"
#include "components/component_transform.h"
#include "entity/entity_system.h"
#include "entity/entity_handle.h"
#include "core/log.h"
#include "core/profiler.h"
#include <PxPhysicsAPI.h>
#include <pvd/PxPvd.h>
#include <pvd/PxPvdTransport.h>
#include <malloc.h>

namespace Engine
{
	class AllocatorCallback : public physx::PxAllocatorCallback
	{
	public:
		virtual ~AllocatorCallback() {}
		virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
		{
			return _aligned_malloc(size, 16);
		}
		virtual void deallocate(void* ptr)
		{
			_aligned_free(ptr);
		}
	} g_physxAllocator;

	class ErrorCallback : public physx::PxErrorCallback
	{
	public:
		virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
		{
			SDE_LOG("PhysX Error!\n\tCode %d\n\t%s\n\t@%s, line %d", code, message, file, line);
		}
	} g_physxErrors;

	class PhysicsSystem::JobDispatcher : public physx::PxCpuDispatcher
	{
	public:
		virtual void submitTask(physx::PxBaseTask& task)
		{
			m_jobs->PushJob([&task](void*) {
				SDE_PROF_EVENT("PhysX Task");
				task.run();
				task.release();
			});
		}

		virtual uint32_t getWorkerCount() const
		{
			return m_jobs->GetThreadCount();
		}

		JobSystem* m_jobs = nullptr;
	} g_dispatcher;

	PhysicsSystem::PhysicsSystem()
	{
	}

	PhysicsSystem::~PhysicsSystem()
	{
	}

	bool PhysicsSystem::PreInit(SystemEnumerator& systemEnumerator)
	{
		SDE_PROF_EVENT();

		m_jobSystem = (Engine::JobSystem*)systemEnumerator.GetSystem("Jobs");
		m_entitySystem = (EntitySystem*)systemEnumerator.GetSystem("Entities");

		m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_physxAllocator, g_physxErrors);

		// Hook up visual debugger
		m_pvd = physx::PxCreatePvd(*m_foundation.Get());
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

		m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation.Get(), physx::PxTolerancesScale(), false, m_pvd.Get());

		physx::PxCookingParams cookingParams(m_physics->getTolerancesScale());
		m_cooker = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation.Get(), cookingParams);
		PxInitExtensions(*m_physics.Get(), m_pvd.Get());

		// Hook up our own job dispatcher and create the scene
		g_dispatcher.m_jobs = m_jobSystem;
		physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = &g_dispatcher;
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
		m_scene = m_physics->createScene(sceneDesc);

		return true;
	}

	bool PhysicsSystem::Initialise()
	{
		SDE_PROF_EVENT();

		m_entitySystem->RegisterComponentType<Physics>();
		m_entitySystem->RegisterComponentUi<Physics>([](ComponentStorage& cs, EntityHandle e, Engine::DebugGuiSystem& dbg) {
			auto& p = *static_cast<Physics::StorageType&>(cs).Find(e);
			auto isStatic = p.IsStatic();
			p.SetStatic(dbg.Checkbox("Static", isStatic));
			p.SetStaticFriction(dbg.DragFloat("Friction (Static)", p.GetStaticFriction(), 0.01f, 0.0f, 10.0f));
			p.SetDynamicFriction(dbg.DragFloat("Friction (Dynamic)", p.GetDynamicFriction(), 0.01f, 0.0f, 10.0f));
			p.SetRestitution(dbg.DragFloat("Restitution", p.GetRestitution(), 0.01f, 0.0f, 10.0f));
			if (dbg.TreeNode("Colliders", true))
			{
				for (auto& it : p.GetPlaneColliders())
				{
					if (dbg.TreeNode("Plane"))
					{
						std::get<0>(it) = dbg.DragVector("Normal", std::get<0>(it), 0.01f, -1.0f, 1.0f);
						std::get<1>(it) = dbg.DragVector("Origin", std::get<1>(it), 0.01f, -1.0f, 1.0f);
						dbg.TreePop();
					}
				}
				for (auto& it : p.GetSphereColliders())
				{
					if (dbg.TreeNode("Sphere"))
					{
						std::get<0>(it) = dbg.DragVector("Offset", std::get<0>(it), 0.01f);
						std::get<1>(it) = dbg.DragFloat("Radius", std::get<1>(it), 0.01f, 0.0f, 100000.0f);
						dbg.TreePop();
					}
				}
				for (auto& it : p.GetBoxColliders())
				{
					if (dbg.TreeNode("Box"))
					{
						std::get<0>(it) = dbg.DragVector("Offset", std::get<0>(it), 0.01f);
						std::get<1>(it) = dbg.DragVector("Dimensions", std::get<1>(it), 0.01f, 0.0f, 100000.0f);
						dbg.TreePop();
					}
				}
				dbg.TreePop();
			}
		});

		return true;
	}

	void PhysicsSystem::RebuildActor(Physics& p, EntityHandle& e)
	{
		auto transformComponent = m_entitySystem->GetWorld()->GetComponent<Transform>(e);
		if (!transformComponent)
		{
			return;
		}

		const auto pos = transformComponent->GetPosition();
		physx::PxTransform trans(physx::PxVec3(pos.x, pos.y, pos.z));	// todo, rotate, scale
		physx::PxMaterial* material = m_physics->createMaterial(p.GetStaticFriction(), p.GetDynamicFriction(), p.GetRestitution());
		physx::PxRigidActor* body = nullptr;
		if (p.IsStatic())
		{
			body = m_physics->createRigidStatic(trans);
		}
		else
		{
			body = m_physics->createRigidDynamic(trans);
		}
	
		for (const auto& collider : p.GetPlaneColliders())
		{
			physx::PxVec3 normal = { std::get<0>(collider).x, std::get<0>(collider).y, std::get<0>(collider).z };
			physx::PxVec3 origin = { std::get<1>(collider).x, std::get<1>(collider).y, std::get<1>(collider).z };
			auto planePose = physx::PxTransformFromPlaneEquation(physx::PxPlane(origin, normal));
			auto shape = m_physics->createShape(physx::PxPlaneGeometry(), *material);
			shape->setLocalPose(planePose);
			body->attachShape(*shape);
		}
		for (const auto& collider : p.GetSphereColliders())
		{
			physx::PxVec3 offset = { std::get<0>(collider).x, std::get<0>(collider).y, std::get<0>(collider).z };
			float radius = std::get<1>(collider);
			auto shape = m_physics->createShape(physx::PxSphereGeometry(radius), *material);
			shape->setLocalPose(physx::PxTransform(offset));
			body->attachShape(*shape);
		}
		for (const auto& collider : p.GetBoxColliders())
		{
			physx::PxVec3 offset = { std::get<0>(collider).x, std::get<0>(collider).y, std::get<0>(collider).z };
			physx::PxVec3 dims = { std::get<1>(collider).x/2.0f, std::get<1>(collider).y/2.0f, std::get<1>(collider).z/2.0f };
			auto shape = m_physics->createShape(physx::PxBoxGeometry(dims.x,dims.y,dims.z), *material);
			shape->setLocalPose(physx::PxTransform(offset));
			body->attachShape(*shape);
		}
		p.SetActor(Engine::PhysicsHandle<physx::PxRigidActor>(body));
		m_scene->addActor(*body);
		p.SetNeedsRebuild(false);
	}

	bool PhysicsSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		// Rebuild actors if needed
		{
			SDE_PROF_EVENT("RebuildPending");
			m_entitySystem->GetWorld()->ForEachComponent<Physics>([this](Physics& p, EntityHandle e)
			{
				if (p.NeedsRebuild())
				{
					RebuildActor(p, e);
				}
			});
		}

		m_timeAccumulator += timeDelta;
		if (m_timeAccumulator >= m_timeStep)
		{
			SDE_PROF_EVENT("Simulate");
			m_timeAccumulator -= m_timeStep;
			m_scene->simulate(m_timeStep);	// Do not write to any physics objects during this!
			m_scene->fetchResults(true);
		}

		// Apply new transforms to all dynamic bodies
		{
			SDE_PROF_EVENT("ApplyTransforms");
			auto transforms = m_entitySystem->GetWorld()->GetAllComponents<Transform>();
			m_entitySystem->GetWorld()->ForEachComponent<Physics>([this,&transforms](Physics& p, EntityHandle e)
			{
				if (!p.IsStatic() && p.GetActor().Get() != nullptr)
				{
					auto transform = transforms->Find(e);
					if (transform)
					{
						const auto& pose = p.GetActor()->getGlobalPose();
						transform->SetPosition({pose.p.x, pose.p.y, pose.p.z});
					}
				}
			});
		}

		return true;
	}

	void PhysicsSystem::Shutdown()
	{
		SDE_PROF_EVENT();

		m_entitySystem->GetWorld()->ForEachComponent<Physics>([this](Physics& p, EntityHandle e)
		{
			p.SetActor(Engine::PhysicsHandle<physx::PxRigidActor>(nullptr));
		});

		m_scene = nullptr;
		m_physics = nullptr;
		m_pvd = nullptr;
		m_foundation = nullptr;
	}
}
