#include "projectile_component.h"

COMPONENT_SCRIPTS(ProjectileComponent,
	"SetMaxDistance", &ProjectileComponent::SetMaxDistance,
	"SetVelocity", &ProjectileComponent::SetVelocity,
	"SetDamageRange", &ProjectileComponent::SetDamageRange,
	"SetRadius", &ProjectileComponent::SetRadius
)
SERIALISE_BEGIN(ProjectileComponent)
SERIALISE_PROPERTY("MaxDistance", m_maxDistance)
SERIALISE_PROPERTY("Velocity", m_velocity)
SERIALISE_PROPERTY("MinHitDamage", m_minHitDamage)
SERIALISE_PROPERTY("MaxHitDamage", m_maxHitDamage)
SERIALISE_PROPERTY("Radius", m_radius)
SERIALISE_PROPERTY("DistanceTravelled", m_distanceTravelled)
SERIALISE_PROPERTY("HitObjects", m_hitObjects)
SERIALISE_END()