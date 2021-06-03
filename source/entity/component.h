#pragma once
#include "engine/serialisation.h"
#include "engine/script_system.h"
#include "component_storage.h"
#include <string>

using ComponentType = std::string;

// add this to the class declaration (make sure its public!)
#define COMPONENT(className)	\
	using StorageType = LinearComponentStorage<className>;	\
	static ComponentType GetType() { return #className; }	\
	static void RegisterScripts(sol::state& s);	\
	SERIALISED_CLASS();

#define COMPONENT_INSPECTOR(...)	\
	static std::function<void(ComponentStorage& cs, const EntityHandle& e)> MakeInspector(__VA_ARGS__);

#define COMPONENT_INSPECTOR_IMPL(className,...)	\
	std::function<void(ComponentStorage& cs, const EntityHandle& e)> className::MakeInspector(__VA_ARGS__)

// Pass script bindings in COMPONENT_SCRIPTS
// (Very thin wrapper around sol, check the sol docs for details)
// Finally declare serialised properties with SERIALISE_BEGIN/END
#define COMPONENT_SCRIPTS(className, ...)	\
	void className::RegisterScripts(sol::state& s)	\
	{	\
		s.new_usertype<className>(#className, sol::constructors<className()>(),	\
			"GetType", &className::GetType,	\
			__VA_ARGS__	\
			);	\
	}

namespace Engine
{
	class DebugGuiSystem;
}
class EntityHandle;