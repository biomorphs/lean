#include "player_component.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(PlayerComponent)

SERIALISE_BEGIN(PlayerComponent)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(PlayerComponent)
{
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto fn = [gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& a = *static_cast<StorageType&>(cs).Find(e);

	};
	return fn;
}