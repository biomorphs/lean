#pragma once
#include "engine/serialisation.h"
#include "entity_handle.h"
#include "component.h"
#include "component_storage.h"
#include "robin_hood.h"
#include <string>

namespace Engine
{
	class ScriptSystem;
}

class EntityHandle;
class World
{
public:
	World() = default;
	~World() = default;
	SERIALISED_CLASS();

	EntityHandle AddEntity();
	void RemoveEntity(EntityHandle h);
	const std::vector<uint32_t>& AllEntities() const { return m_allEntities; }
	void RemoveAllEntities();

	template<class ComponentType>
	void RegisterComponentType(Component::Type);

	template<class ComponentType>
	void ForEachComponent(Component::Type t,std::function<void(Component&, EntityHandle)> fn);
	
	Component* AddComponent(EntityHandle owner, Component::Type type);
	void RemoveComponent(EntityHandle owner, Component::Type type);
	ComponentStorage* GetComponentStorage(Component::Type type);
	Component* GetComponent(EntityHandle owner, Component::Type type);
	std::vector<Component*> GetAllComponents(EntityHandle owner);

private:
	uint32_t m_entityIDCounter;
	std::vector<uint32_t> m_allEntities;	// all active entity IDs
	robin_hood::unordered_map<Component::Type, std::unique_ptr<ComponentStorage>> m_components;	// all component data
};

template<class ComponentType>
void World::RegisterComponentType(Component::Type t)
{
	assert(m_components.find(t) == m_components.end());
	auto storage = std::make_unique<LinearComponentStorage<ComponentType>>();
	m_components[t] = std::move(storage);
}

template<class ComponentType>
void World::ForEachComponent(Component::Type t, std::function<void(Component&, EntityHandle)> fn)
{
	auto foundStorage = m_components.find(t);
	if (foundStorage != m_components.end())
	{
		return foundStorage->second->ForEach(fn);
	}
}