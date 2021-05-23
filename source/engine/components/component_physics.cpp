#include "component_physics.h"
#include <PxPhysicsAPI.h>

COMPONENT_SCRIPTS(Physics,
	"IsStatic", &Physics::IsStatic,
	"SetStatic", &Physics::SetStatic,
	"GetStaticFriction", &Physics::GetStaticFriction,
	"SetStaticFriction", &Physics::SetStaticFriction,
	"SetDynamicFriction", &Physics::SetDynamicFriction,
	"GetDynamicFriction", &Physics::GetDynamicFriction,
	"SetRestitution", &Physics::SetRestitution,
	"GetRestitution", &Physics::GetRestitution,
	"AddPlaneCollider", &Physics::AddPlaneCollider,
	"AddSphereCollider", &Physics::AddSphereCollider,
	"Rebuild", &Physics::Rebuild
)

Physics::Physics()
{

}

Physics::~Physics()
{

}