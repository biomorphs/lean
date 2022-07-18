#include "monster_component.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(MonsterComponent,
	"GetCurrentHealth", &MonsterComponent::GetCurrentHealth,
	"GetSpeed", &MonsterComponent::GetSpeed,
	"SetSpeed", &MonsterComponent::SetSpeed,
	"GetVisionRadius", &MonsterComponent::GetVisionRadius,
	"SetVisionRadius", &MonsterComponent::SetVisionRadius,
	"GetDespawnRadius", &MonsterComponent::GetDespawnRadius,
	"SetDespawnRadius", &MonsterComponent::SetDespawnRadius,
	"GetCollideRadius", &MonsterComponent::GetCollideRadius,
	"SetCollideRadius", &MonsterComponent::SetCollideRadius
)
SERIALISE_BEGIN(MonsterComponent)
SERIALISE_PROPERTY("CurrentHP", m_currentHP)
SERIALISE_PROPERTY("Speed", m_speed)
SERIALISE_PROPERTY("VisionRadius", m_visionRadius)
SERIALISE_PROPERTY("DespawnRadius", m_despawnRadius)
SERIALISE_PROPERTY("CollideRadius", m_collideRadius)
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
	};
	return fn;
}