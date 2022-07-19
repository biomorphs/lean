#include "monster_component.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"
#include "core/random.h"

COMPONENT_SCRIPTS(MonsterComponent,
	"GetCurrentHealth", &MonsterComponent::GetCurrentHealth,
	"GetSpeed", &MonsterComponent::GetSpeed,
	"SetSpeed", &MonsterComponent::SetSpeed,
	"GetVisionRadius", &MonsterComponent::GetVisionRadius,
	"SetVisionRadius", &MonsterComponent::SetVisionRadius,
	"GetDespawnRadius", &MonsterComponent::GetDespawnRadius,
	"SetDespawnRadius", &MonsterComponent::SetDespawnRadius,
	"GetCollideRadius", &MonsterComponent::GetCollideRadius,
	"SetCollideRadius", &MonsterComponent::SetCollideRadius,
	"AddKnockback", &MonsterComponent::AddKnockback,
	"SetKnockbackFalloff", &MonsterComponent::SetKnockbackFalloff,
	"GetKnockbackFalloff", &MonsterComponent::GetKnockbackFalloff
)
SERIALISE_BEGIN(MonsterComponent)
SERIALISE_PROPERTY("CurrentHP", m_currentHP)
SERIALISE_PROPERTY("Speed", m_speed)
SERIALISE_PROPERTY("VisionRadius", m_visionRadius)
SERIALISE_PROPERTY("DespawnRadius", m_despawnRadius)
SERIALISE_PROPERTY("CollideRadius", m_collideRadius)
SERIALISE_PROPERTY("Knockback", m_knockBack)
SERIALISE_PROPERTY("KnockbackFalloff", m_knockBackFalloff)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(MonsterComponent)
{
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto fn = [gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& a = *static_cast<StorageType&>(cs).Find(e);
		char text[1024];
		sprintf_s(text, "Current HP: %d", a.GetCurrentHealth());
		gui->Text(text);
		i.Inspect("Move Speed", a.GetSpeed(), InspectFn(e, &MonsterComponent::SetSpeed));
		i.Inspect("Vision Radius", a.GetVisionRadius(), InspectFn(e, &MonsterComponent::SetVisionRadius));
		i.Inspect("Despawn Radius", a.GetDespawnRadius(), InspectFn(e, &MonsterComponent::SetDespawnRadius));
		i.Inspect("Collision Radius", a.GetCollideRadius(), InspectFn(e, &MonsterComponent::SetCollideRadius));
		i.Inspect("Knockback falloff", a.GetKnockbackFalloff(), InspectFn(e, &MonsterComponent::SetKnockbackFalloff));
		if (gui->Button("Random knockback"))
		{
			glm::vec2 force = {
				Core::Random::GetFloat(-64.0f,64.0f),
				Core::Random::GetFloat(-64.0f,64.0f)
			};
			a.AddKnockback(force);
		}
	};
	return fn;
}