#pragma once
#include "engine/system.h"
#include "world.h"
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
	bool PreInit(Engine::SystemManager&);
	bool Initialise();
	bool Tick(float timeDelta);
	void Shutdown();

	template<class T>
	void RegisterComponentType();

	using UiRenderFn = std::function<void(ComponentStorage&, EntityHandle, Engine::DebugGuiSystem&)>;
	template<class ComponentType>
	void RegisterComponentUi(UiRenderFn fn);

	World* GetWorld() { return m_world.get(); }
	void NewWorld();

	std::string GetEntityNameWithTags(EntityHandle e) const;

private:
	void ShowDebugGui();
	std::map<ComponentType, UiRenderFn> m_componentUiRenderers;
	std::unique_ptr<World> m_world;
	Engine::ScriptSystem* m_scriptSystem;
	Engine::DebugGuiSystem* m_debugGui;
};

template<class ComponentType>
void EntitySystem::RegisterComponentUi(UiRenderFn fn)
{
	m_componentUiRenderers[ComponentType::GetType()] = fn;
}

template<class T>
inline void EntitySystem::RegisterComponentType()
{
	m_world->RegisterComponentType<T>();
	T::RegisterScripts(m_scriptSystem->Globals());

	auto world = m_scriptSystem->Globals()["World"].get_or_create<sol::table>();
	world["AddComponent_" + T::GetType()] = [this](EntityHandle h) -> T*
	{
		m_world->AddComponent(h, T::GetType());
		return m_world->GetComponent<T>(h);
	};
	world["GetComponent_" + T::GetType()] = [this](EntityHandle h) -> T*
	{
		return static_cast<T*>(m_world->GetComponent<T>(h));
	};
}