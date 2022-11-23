#include "dead_monster_component.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(DeadMonsterComponent,
	"GetDeathTime", &DeadMonsterComponent::GetDeathTime,
	"SetDeathTime", &DeadMonsterComponent::SetDeathTime
)

SERIALISE_BEGIN(DeadMonsterComponent)
SERIALISE_PROPERTY("DeathTime", m_deathTime)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(DeadMonsterComponent)
{
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto fn = [gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& a = *static_cast<StorageType&>(cs).Find(e);
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		i.Inspect("Death Time", a.GetDeathTime(), InspectFn(e, &DeadMonsterComponent::SetDeathTime));
	};
	return fn;
}