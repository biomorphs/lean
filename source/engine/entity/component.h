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