#pragma once
#include "engine/serialisation.h"
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
	static void RegisterScripts(Engine::ScriptSystem&);

	using Type = std::string;
	virtual Type GetType() const { return "Component"; }
};

// helper macros
#define COMPONENT(className)	\
	virtual Type GetType() const { return #className; }	\
	static void RegisterScripts(Engine::ScriptSystem& s);	\
	virtual SERIALISED_CLASS();

// Pass script bindings in COMPONENT_BEGIN
// Define serialised properties between begin/end
// e.g.
// COMPONENT_BEGIN(Test, "SomeFunction", &Test::SomeFn)
// SERIALISE_PROPERTY("SomeProp", m_property)
// COMPONENT_END()

#define COMPONENT_BEGIN(className, ...)	\
	void className::RegisterScripts(Engine::ScriptSystem& s)	\
	{	\
		s.Globals().new_usertype<className>(#className, sol::constructors<className()>(),	\
			"GetType", &Component::GetType,	\
			__VA_ARGS__	\
			);	\
	}	\
	SERIALISE_BEGIN_WITH_PARENT(className,Component)	\

#define COMPONENT_END()	\
	SERIALISE_END()