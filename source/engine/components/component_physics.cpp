#include "component_physics.h"
#include <PxPhysicsAPI.h>

COMPONENT_SCRIPTS(Physics,
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