#include "physics_system.h"
#include "debug_gui_system.h"
#include "job_system.h"
#include "graphics_system.h"
#include "debug_render.h"
#include "components/component_physics.h"
#include "components/component_transform.h"
#include "engine/system_manager.h"
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
	PhysicsSystem::UpdateEntities::UpdateEntities(PhysicsSystem* physics)
		: m_physicsSystem(physics)
	{
	}

	bool PhysicsSystem::UpdateEntities::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		auto entities = m_physicsSystem->m_entitySystem;
		if(m_physicsSystem->m_hasTicked)
		{ 
			// wait for sim to end
			{
				SDE_PROF_EVENT("FetchResults");
				m_physicsSystem->m_scene->fetchResults(true);	
			}

			static bool s_useActiveActors = true;

			if (s_useActiveActors)
			{
				SDE_PROF_EVENT("ApplyTransforms_ActiveActors");
				// Apply new transforms to all non-kinematic dynamic bodies
				auto theWorld = m_physicsSystem->m_entitySystem->GetWorld();
				auto transforms = theWorld->GetAllComponents<Transform>();
				auto physicscomps = theWorld->GetAllComponents<Physics>();
				physx::PxU32 nbActiveActors = 0;
				physx::PxActor** activeActors = m_physicsSystem->m_scene->getActiveActors(nbActiveActors);
				m_physicsSystem->m_jobSystem->ForEachAsync(0, nbActiveActors, 1, 400, [this, &activeActors, &transforms, &physicscomps](int32_t i) 
				{
					auto entityId = reinterpret_cast<uintptr_t>(activeActors[i]->userData);
					auto p = physicscomps->Find(entityId);
					auto t = transforms->Find(entityId);
					if (p && t && !p->IsStatic() && !p->IsKinematic() && p->GetActor().Get() != nullptr)
					{
						const auto& pose = p->GetActor()->getGlobalPose();
						t->SetPosition({ pose.p.x, pose.p.y, pose.p.z });
						t->SetOrientation({ pose.q.w, pose.q.x, pose.q.y, pose.q.z });
					}
				});
			}
			else
			{
				// we should not be doing this by looping through all components!
				{
					SDE_PROF_EVENT("ApplyTransforms_AllComponents");
					auto transforms = entities->GetWorld()->GetAllComponents<Transform>();
					entities->GetWorld()->ForEachComponentAsync<Physics>([this, &transforms](Physics& p, EntityHandle e)
						{
							if (!p.IsStatic() && !p.IsKinematic() && p.GetActor().Get() != nullptr)
							{
								auto transform = transforms->Find(e);
								if (transform)
								{
									const auto& pose = p.GetActor()->getGlobalPose();
									transform->SetPosition({ pose.p.x, pose.p.y, pose.p.z });
									transform->SetOrientation({ pose.q.w, pose.q.x, pose.q.y, pose.q.z });
								}
							}
						}, *m_physicsSystem->m_jobSystem, 400);
				}
			}
			m_physicsSystem->m_hasTicked = false;
		}
		
		// Rebuild actors if needed
		// we should not be doing this by looping through all components!
		{
			SDE_PROF_EVENT("RebuildPending");
			entities->GetWorld()->ForEachComponent<Physics>([this,entities](Physics& p, EntityHandle e)
			{
				if (p.NeedsRebuild())
				{
					m_physicsSystem->RebuildActor(p, e);
				}
				// update kinematic target transforms
				if (!p.IsStatic() && p.IsKinematic() && p.GetActor().Get() != nullptr)
				{
					auto transformComponent = entities->GetWorld()->GetComponent<Transform>(e);
					if (transformComponent)
					{
						const auto pos = transformComponent->GetPosition();
						const auto orient = transformComponent->GetOrientation();
						physx::PxTransform trans(physx::PxVec3(pos.x, pos.y, pos.z), physx::PxQuat(orient.x, orient.y, orient.z, orient.w));
						static_cast<physx::PxRigidDynamic*>(p.GetActor().Get())->setKinematicTarget(trans);
					}
				}
			});
		}

		return true;
	}

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

	PhysicsSystem::UpdateEntities* PhysicsSystem::MakeUpdater()
	{
		return new UpdateEntities(this);
	}

	bool PhysicsSystem::PreInit(SystemManager& manager)
	{
		SDE_PROF_EVENT();

		m_jobSystem = (Engine::JobSystem*)manager.GetSystem("Jobs");
		m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");
		m_graphicsSystem = (GraphicsSystem*)manager.GetSystem("Graphics");
		m_debugGuiSystem = (DebugGuiSystem*)manager.GetSystem("DebugGui");

		m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_physxAllocator, g_physxErrors);

		// Hook up visual debugger
		m_pvd = physx::PxCreatePvd(*m_foundation.Get());
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

		m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation.Get(), physx::PxTolerancesScale(), false, m_pvd.Get());

		physx::PxCookingParams cookingParams(m_physics->getTolerancesScale());
		m_cooker = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation.Get(), cookingParams);
		PxInitExtensions(*m_physics.Get(), m_pvd.Get());

		// Hook up our own job dispatcher, CUDA and create the scene
		physx::PxCudaContextManagerDesc cudaContextManagerDesc;
		m_cudaManager = PxCreateCudaContextManager(*m_foundation.Get(), cudaContextManagerDesc, PxGetProfilerCallback());

		const bool c_useCUDA = true;
		g_dispatcher.m_jobs = m_jobSystem;
		physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = &g_dispatcher;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;					// generate active actor list
		sceneDesc.flags |= physx::PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;	// ... but don't include kinematics
		if (c_useCUDA)
		{
			sceneDesc.cudaContextManager = m_cudaManager.Get();
			sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
			sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
			sceneDesc.gpuDynamicsConfig.patchStreamSize = 1024 * 128;		// required for big scenes
		}
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
		m_scene = m_physics->createScene(sceneDesc);

		return true;
	}

	bool PhysicsSystem::Initialise()
	{
		SDE_PROF_EVENT();

		m_entitySystem->RegisterComponentType<Physics>();
		auto& dbgRender = m_graphicsSystem->DebugRenderer();
		auto& world = *m_entitySystem->GetWorld();
		m_entitySystem->RegisterInspector<Physics>(Physics::MakeInspector(*m_debugGuiSystem, dbgRender, world));

		return true;
	}

	physx::PxMaterial* PhysicsSystem::GetOrCreateMaterial(Physics& p)
	{
		// Annoying, physx has limited material slots. I dont want to expose them
		// so instead we keep an internal cache of materials with matching values
		auto foundMaterial = std::find_if(m_materialCache.begin(), m_materialCache.end(), [&p](const PhysicsMaterial& mat)
		{
			return mat.m_staticFriction == p.GetStaticFriction() && mat.m_dynamicFriction == p.GetDynamicFriction() && mat.m_restitution == p.GetRestitution();
		});
		if (foundMaterial == m_materialCache.end())
		{
			auto material = m_physics->createMaterial(p.GetStaticFriction(), p.GetDynamicFriction(), p.GetRestitution());
			auto newHandle = PhysicsHandle<physx::PxMaterial>(material);
			m_materialCache.push_back({ p.GetRestitution(), p.GetStaticFriction(), p.GetDynamicFriction(), std::move(newHandle) });
			return material;
		}
		else
		{
			return foundMaterial->m_handle.Get();
		}
	}

	void PhysicsSystem::RebuildActor(Physics& p, EntityHandle& e)
	{
		auto transformComponent = m_entitySystem->GetWorld()->GetComponent<Transform>(e);
		if (!transformComponent)
		{
			return;
		}
		const auto pos = transformComponent->GetPosition();
		const auto orient = transformComponent->GetOrientation();
		physx::PxTransform trans(physx::PxVec3(pos.x, pos.y, pos.z), physx::PxQuat(orient.x, orient.y, orient.z, orient.w));		
		physx::PxRigidActor* body = nullptr;
		if (p.IsStatic())
		{
			body = m_physics->createRigidStatic(trans);
		}
		else
		{
			body = m_physics->createRigidDynamic(trans);
			static_cast<physx::PxRigidDynamic*>(body)->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, p.IsKinematic());
		}
		body->userData = reinterpret_cast<void*>((uintptr_t)e.GetID());	// keep a reference to the entity, not the component 
	
		auto material = GetOrCreateMaterial(p);
		for (const auto& collider : p.GetPlaneColliders())
		{
			physx::PxVec3 normal = { std::get<0>(collider).x, std::get<0>(collider).y, std::get<0>(collider).z };
			physx::PxVec3 origin = { std::get<1>(collider).x, std::get<1>(collider).y, std::get<1>(collider).z };
			auto planePose = physx::PxTransformFromPlaneEquation(physx::PxPlane(origin, normal));
			auto shape = m_physics->createShape(physx::PxPlaneGeometry(), *material, true);
			shape->setLocalPose(planePose);
			body->attachShape(*shape);
		}
		for (const auto& collider : p.GetSphereColliders())
		{
			physx::PxVec3 offset = { std::get<0>(collider).x, std::get<0>(collider).y, std::get<0>(collider).z };
			float radius = std::get<1>(collider);
			auto shape = m_physics->createShape(physx::PxSphereGeometry(radius), *material, true);
			shape->setLocalPose(physx::PxTransform(offset));
			body->attachShape(*shape);
		}
		for (const auto& collider : p.GetBoxColliders())
		{
			physx::PxVec3 offset = { std::get<0>(collider).x, std::get<0>(collider).y, std::get<0>(collider).z };
			physx::PxVec3 dims = { std::get<1>(collider).x/2.0f, std::get<1>(collider).y/2.0f, std::get<1>(collider).z/2.0f };
			auto shape = m_physics->createShape(physx::PxBoxGeometry(dims.x,dims.y,dims.z), *material, true);
			shape->setLocalPose(physx::PxTransform(offset));
			body->attachShape(*shape);
		}
		if (!p.IsStatic())
		{
			physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(body), p.GetDensity());
		}
		p.SetActor(Engine::PhysicsHandle<physx::PxRigidActor>(body));
		m_scene->addActor(*body);
		p.SetNeedsRebuild(false);
	}

	bool PhysicsSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		m_timeAccumulator += timeDelta;
		if (m_timeAccumulator >= m_timeStep)
		{
			SDE_PROF_EVENT("Simulate");
			m_timeAccumulator -= m_timeStep;
			m_scene->simulate(m_timeStep);	// Do not write to any physics objects until UpdateEntities has ticked!
			m_hasTicked = true;
		}

		return true;
	}

	void PhysicsSystem::Shutdown()
	{
		SDE_PROF_EVENT();

		if (m_hasTicked)
		{
			m_scene->fetchResults(true);
		}

		m_materialCache.clear();
		m_scene = nullptr;
		m_physics = nullptr;
		m_pvd = nullptr;
		m_cudaManager = nullptr;
		m_cooker = nullptr;
		m_foundation = nullptr;
	}
}
