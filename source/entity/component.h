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
	Engine::Serialisation::ObjectFactory<ComponentStorage>::Register \
		s_registerFactory_parent_LCS_##className("LinearComponentStorage<" #className ">",[]() {	\
		return reinterpret_cast<ComponentStorage*>(new LinearComponentStorage<className>);	\
	});	\
	void LinearComponentStorage<className>::Serialise(nlohmann::json& json, Engine::SerialiseType op)	\
	{	\
		if(op==Engine::SerialiseType::Write) {		\
			std::unordered_map<uint32_t,uint32_t> m;	\
			for(const auto& it : m_entityToComponent)	\
				m[it.first] = it.second;				\
			json["EntityToComponent"] = m;				\
			Engine::ToJson("Owners", m_owners, json);	\
			Engine::ToJson("Components", m_components, json);	\
			Engine::ToJson("ClassName", "LinearComponentStorage<" #className ">", json);	\
		}	\
		else if(op==Engine::SerialiseType::Read) {		\
			std::unordered_map<uint32_t,uint32_t> m = json["EntityToComponent"];	\
			for(auto it : m)	m_entityToComponent[it.first] = it.second;	\
			Engine::FromJson("Owners", m_owners, json);	\
			Engine::FromJson("Components", m_components, json);	\
		}	\
	}	\
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