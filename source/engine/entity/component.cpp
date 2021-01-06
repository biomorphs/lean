#include "component.h"
#include "engine/script_system.h"

SERIALISE_BEGIN(Component)
SERIALISE_END()

void Component::RegisterScripts(sol::state& s)
{
	s.new_usertype<Component>("Component", sol::constructors<Component()>(),
										"GetType", &Component::GetType);
}