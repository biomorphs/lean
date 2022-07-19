#include "explosion_component.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(ExplosionComponent,
	"SetDamageRadius", &ExplosionComponent::SetDamageRadius,
	"SetDamageAtCenter", &ExplosionComponent::SetDamageAtCenter,
	"SetDamageAtEdge", &ExplosionComponent::SetDamageAtEdge,
	"SetFadeoutSpeed", &ExplosionComponent::SetFadeoutSpeed
)
SERIALISE_BEGIN(ExplosionComponent)
SERIALISE_PROPERTY("HasExploded", m_hasExploded)
SERIALISE_PROPERTY("DamageRadius", m_damageRadius)
SERIALISE_PROPERTY("DamageAtCenter", m_damageAtCenter)
SERIALISE_PROPERTY("DamageAtEdge", m_damageAtEdge)
SERIALISE_PROPERTY("FadeOutSpeed", m_fadeOutSpeed)
SERIALISE_END()