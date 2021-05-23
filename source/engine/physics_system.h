#pragma once

#include "system.h"
#include "physics_handle.h"
#include <memory>

namespace physx
{
	class PxFoundation;
	class PxPvd;
	class PxPhysics;
	class PxScene;
	class PxCpuDispatcher;
	class PxCooking;
}

class EntitySystem;
class EntityHandle;
class Physics;
namespace Engine
{
	class JobSystem;
	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem();
		virtual ~PhysicsSystem();

		bool PreInit(SystemEnumerator& systemEnumerator);
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();

	private:
		void RebuildActor(Physics& p, EntityHandle& e);

		class JobDispatcher;
		JobSystem* m_jobSystem = nullptr;
		EntitySystem* m_entitySystem = nullptr;
		float m_timeAccumulator = 0.0f;
		float m_timeStep = 1.0f / 60.0f;	// fixed time step for now
		PhysicsHandle<physx::PxFoundation> m_foundation;
		PhysicsHandle<physx::PxPvd> m_pvd;
		PhysicsHandle<physx::PxPhysics> m_physics;
		PhysicsHandle<physx::PxCooking> m_cooker;
		PhysicsHandle<physx::PxScene> m_scene;
	};
}
