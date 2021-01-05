#pragma once
#include "engine/serialisation.h"
#include "entity_handle.h"
#include "component.h"
#include "component_storage.h"
#include <unordered_map>
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

	template<class ComponentType>
	void RegisterComponentType(Component::Type);
	
	Component* AddComponent(EntityHandle owner, Component::Type type);
	void RemoveComponent(EntityHandle owner, Component::Type type);
	Component* GetComponent(EntityHandle owner, Component::Type type);
	std::vector<Component*> GetAllComponents(EntityHandle owner);

private:
	uint32_t m_entityIDCounter;
	std::vector<uint32_t> m_allEntities;	// all active entity IDs
	std::unordered_map<Component::Type, std::unique_ptr<ComponentStorage>> m_components;	// all component data
};

template<class ComponentType>
void World::RegisterComponentType(Component::Type t)
{
	assert(m_components.find(t) == m_components.end());
	auto storage = std::make_unique<LinearComponentStorage<ComponentType>>();
	m_components[t] = std::move(storage);
}