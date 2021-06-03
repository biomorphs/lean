#include "component_physics.h"
#include <PxPhysicsAPI.h>

COMPONENT_SCRIPTS(Physics,
	"AddForce", &Physics::AddForce,
	"IsStatic", &Physics::IsStatic,
	"SetStatic", &Physics::SetStatic,
	"SetKinematic", &Physics::SetKinematic,
	"IsKinematic", &Physics::IsKinematic,
	"SetDensity", &Physics::SetDensity,
	"GetStaticFriction", &Physics::GetStaticFriction,
	"SetStaticFriction", &Physics::SetStaticFriction,
	"SetDynamicFriction", &Physics::SetDynamicFriction,
	"GetDynamicFriction", &Physics::GetDynamicFriction,
	"SetRestitution", &Physics::SetRestitution,
	"GetRestitution", &Physics::GetRestitution,
	"AddPlaneCollider", &Physics::AddPlaneCollider,
	"AddSphereCollider", &Physics::AddSphereCollider,
	"AddBoxCollider", &Physics::AddBoxCollider,
	"Rebuild", &Physics::Rebuild
)

Physics::Physics()
{

}

Physics::~Physics()
{
	m_actor = nullptr;
}

void Physics::AddForce(glm::vec3 force)
{
	assert(!IsKinematic() && !IsStatic() && m_actor.Get() != nullptr);
	if (!IsKinematic() && !IsStatic() && m_actor.Get() != nullptr)
	{
		physx::PxVec3 fv(force.x, force.y, force.z);
		static_cast<physx::PxRigidDynamic*>(m_actor.Get())->addForce(fv, physx::PxForceMode::eFORCE);
	}
}