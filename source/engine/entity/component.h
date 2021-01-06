#pragma once
#include "engine/serialisation.h"
#include <sol.hpp>
#include <string>

namespace Engine
{
	class ScriptSystem;
}

class Component
{
public:
	Component() = default;
	virtual ~Component() = default;
	virtual SERIALISED_CLASS();
	static void RegisterScripts(sol::state&);

	using Type = std::string;
	virtual Type GetType() const { return "Component"; }
};

// add this to the class declaration (make sure its public!)
#define COMPONENT(className)	\
	virtual Type GetType() const { return #className; }	\
	static void RegisterScripts(sol::state& s);	\
	virtual SERIALISED_CLASS();

// Pass script bindings in COMPONENT_BEGIN
// Define serialised properties between begin/end
// e.g.
// COMPONENT_BEGIN(Test, "SomeFunction", &Test::SomeFn)
// SERIALISE_PROPERTY("SomeProp", m_property)
// COMPONENT_END()

#define COMPONENT_BEGIN(className, ...)	\
	void className::RegisterScripts(sol::state& s)	\
	{	\
		s.new_usertype<className>(#className, sol::constructors<className()>(),	\
			"GetType", &Component::GetType,	\
			__VA_ARGS__	\
			);	\
	}	\
	SERIALISE_BEGIN_WITH_PARENT(className,Component)	\

#define COMPONENT_END()	\
	SERIALISE_END()