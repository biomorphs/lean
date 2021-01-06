#pragma once
#include "engine/system.h"
#include "component.h"
#include <memory>

namespace Engine
{
	class ScriptSystem;
	class DebugGuiSystem;
}

class World;
class EntitySystem : public Engine::System
{
public:
	EntitySystem();
	virtual ~EntitySystem() = default;
	bool PreInit(Engine::SystemEnumerator&);
	bool Initialise();
	bool Tick();
	void Shutdown();

	template<class T>
	void RegisterComponentType(Component::Type type);

private:
	void ShowDebugGui();
	std::unique_ptr<World> m_world;
	Engine::ScriptSystem* m_scriptSystem;
	Engine::DebugGuiSystem* m_debugGui;
};

#include "world.h"

template<class T>
inline void EntitySystem::RegisterComponentType(Component::Type type)
{
	m_world->RegisterComponentType<T>(type);
	T::RegisterScripts(*m_scriptSystem);

	auto world = m_scriptSystem->Globals()["World"].get_or_create<sol::table>();
	world["AddComponent_" + type] = [this, type](EntityHandle h) -> T*
	{
		return static_cast<T*>(m_world->AddComponent(h, type));
	};
	world["GetComponent_" + type] = [this, type](EntityHandle h) -> T*
	{
		return static_cast<T*>(m_world->GetComponent(h, type));
	};
}