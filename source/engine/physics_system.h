#pragma once

#include "system.h"
#include "physics_handle.h"
#include <memory>
#include <vector>

namespace physx
{
	class PxFoundation;
	class PxPvd;
	class PxPhysics;
	class PxScene;
	class PxCpuDispatcher;
	class PxCooking;
	class PxCudaContextManager;
	class PxMaterial;
}

class EntitySystem;
class EntityHandle;
class Physics;
class GraphicsSystem;
namespace Engine
{
	class JobSystem;

	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem();
		virtual ~PhysicsSystem();

		bool PreInit(SystemManager& manager);
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();
		
		class UpdateEntities : public System
		{
		public:
			UpdateEntities(PhysicsSystem* p);
			bool Tick(float timeDelta);
		private:
			PhysicsSystem* m_physicsSystem = nullptr;
		};
		UpdateEntities* MakeUpdater();

	private:
		void RebuildActor(Physics& p, EntityHandle& e);
		physx::PxMaterial* GetOrCreateMaterial(Physics&);

		class JobDispatcher;
		JobSystem* m_jobSystem = nullptr;
		EntitySystem* m_entitySystem = nullptr;
		GraphicsSystem* m_graphicsSystem = nullptr;
		float m_timeAccumulator = 0.0f;
		float m_timeStep = 1.0f / 60.0f;	// fixed time step for now
		bool m_hasTicked = false;

		// Since physx has a 64k material limit, we will cache materials with the same values
		// later on we probably want some kind of proper physics material exposed
		struct PhysicsMaterial 
		{
			float m_restitution = 0.5f;
			float m_staticFriction = 0.5f;
			float m_dynamicFriction = 0.5f;
			PhysicsHandle<physx::PxMaterial> m_handle;
		};
		std::vector<PhysicsMaterial> m_materialCache;
		PhysicsHandle<physx::PxFoundation> m_foundation;
		PhysicsHandle<physx::PxPvd> m_pvd;
		PhysicsHandle<physx::PxPhysics> m_physics;
		PhysicsHandle<physx::PxCooking> m_cooker;
		PhysicsHandle<physx::PxCudaContextManager> m_cudaManager;
		PhysicsHandle<physx::PxScene> m_scene;
	};
}
