#include "physics_handle.h"
#include <PxPhysicsAPI.h>

namespace Engine
{
	template<class T>
	PhysicsHandle<T>& PhysicsHandle<T>::operator=(T* ptr)
	{
		if (m_v)
		{
			m_v->release();
		}
		m_v = ptr;
		return *this;
	}

	template<class T>
	PhysicsHandle<T>::~PhysicsHandle()
	{
		if (m_v)
		{
			m_v->release();
		}
	}

	template<class T>
	PhysicsHandle<T>::PhysicsHandle(PhysicsHandle&& other)
	{
		m_v = other.m_v;
		other.m_v = nullptr;
	}

	template<class T>
	PhysicsHandle<T>& PhysicsHandle<T>::operator=(PhysicsHandle&& other)
	{
		if (m_v)
		{
			m_v->release();
		}
		m_v = other.m_v;
		other.m_v = nullptr;
		return *this;
	}

	template class PhysicsHandle<physx::PxMaterial>;
	template class PhysicsHandle<physx::PxFoundation>;
	template class PhysicsHandle<physx::PxPvd>;
	template class PhysicsHandle<physx::PxPhysics>;
	template class PhysicsHandle<physx::PxCooking>;
	template class PhysicsHandle<physx::PxCudaContextManager>;
	template class PhysicsHandle<physx::PxScene>;
	template class PhysicsHandle<physx::PxRigidActor>;
}