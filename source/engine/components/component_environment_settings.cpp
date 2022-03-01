#include "component_environment_settings.h"
#include "entity/component_inspector.h"

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

COMPONENT_INSPECTOR_IMPL(EnvironmentSettings)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto c = static_cast<StorageType&>(cs).Find(e);
		i.InspectColour("Clear Colour", c->GetClearColour(), InspectFn(e, &EnvironmentSettings::SetClearColour));
		i.Inspect("Gravity", c->GetGravity(), InspectFn(e, &EnvironmentSettings::SetGravity));
	};
	return fn;
}