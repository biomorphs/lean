#include "component_environment_settings.h"
#include "engine/debug_gui_system.h"
#include "entity/entity_handle.h"

COMPONENT_SCRIPTS(EnvironmentSettings,
	"SetClearColour", &EnvironmentSettings::SetClearColour,
	"GetClearColour", &EnvironmentSettings::GetClearColour,
	"SetGravity", &EnvironmentSettings::SetGravity,
	"GetGravity", &EnvironmentSettings::GetGravity
)

SERIALISE_BEGIN(EnvironmentSettings)
SERIALISE_PROPERTY("ClearColour", m_clearColour)
SERIALISE_PROPERTY("Gravity", m_gravity)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(EnvironmentSettings, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& c = *static_cast<StorageType&>(cs).Find(e);
		c.SetClearColour(gui.ColourEdit("Clear Colour", c.GetClearColour()));
		c.SetGravity(gui.DragVector("Gravity", c.GetGravity(), 0.01f));
	};
	return fn;
}