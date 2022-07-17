#pragma once
#include "engine/system.h"
#include "engine/tag.h"
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
	bool PreInit();
	bool Initialise();
	bool Tick(float timeDelta);
	void Shutdown();

	template<class T>
	void RegisterComponentType();

	// inspector returns entity id to delete or invalid handle if all good
	using InspectorFn = std::function<void(class ComponentInspector&, ComponentStorage&, EntityHandle)>;
	template<class ComponentType> 
	void RegisterInspector(InspectorFn fn);
	uint32_t ShowInspector(const std::vector<uint32_t>& entities, bool expandAll=false, const char* titleText = "Entities", bool allowAddEntity=true, ComponentInspector* i=nullptr);

	World* GetWorld() { return m_world.get(); }
	void NewWorld();

	EntityHandle CloneEntity(EntityHandle src);

	nlohmann::json SerialiseEntities(const std::vector<uint32_t>& entityIDs);
	std::vector<uint32_t> SerialiseEntities(nlohmann::json& data, bool restoreIDsFromData=false);

	std::string GetEntityNameWithTags(EntityHandle e) const;
	EntityHandle GetFirstEntityWithTag(Engine::Tag tag);

private:
	void ShowStats();

	std::map<ComponentType, InspectorFn> m_componentInspectors;
	std::unique_ptr<World> m_world;
	Engine::ScriptSystem* m_scriptSystem;
	Engine::DebugGuiSystem* m_debugGui;
	bool m_showStats = false;
};

template<class ComponentType>
void EntitySystem::RegisterInspector(InspectorFn fn)
{
	m_componentInspectors[ComponentType::GetType()] = fn;
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