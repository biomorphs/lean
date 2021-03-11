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
	void RegisterComponentType();

	template<class ComponentType>
	void ForEachComponent(std::function<void(ComponentType&, EntityHandle)> fn);
	
	template<class ComponentType>
	ComponentType* AddComponent(EntityHandle owner);

	template<class ComponentType>
	ComponentType* GetComponent(EntityHandle owner);

	template<class ComponentType>
	typename ComponentType::StorageType* GetAllComponents();

	ComponentStorage* GetStorage(ComponentType t);

	std::vector<ComponentType> GetOwnedComponentTypes(EntityHandle owner);

	template<class ComponentType>
	void RemoveComponent(EntityHandle owner);

private:
	uint32_t m_entityIDCounter;
	std::vector<uint32_t> m_allEntities;	// all active entity IDs
	robin_hood::unordered_map<ComponentType, std::unique_ptr<ComponentStorage>> m_components;	// all component data
};

template<class ComponentType>
void World::RegisterComponentType()
{
	assert(m_components.find(t) == m_components.end());
	auto storage = std::make_unique<typename ComponentType::StorageType>();
	m_components[ComponentType::GetType()] = std::move(storage);
}

template<class ComponentType>
void World::ForEachComponent(std::function<void(ComponentType&, EntityHandle)> fn)
{
	auto foundStorage = m_components.find(ComponentType::GetType());
	if (foundStorage != m_components.end())
	{
		return static_cast<ComponentType::StorageType*>(foundStorage->second.get())->ForEach(fn);
	}
}

template<class ComponentType>
void World::RemoveComponent(EntityHandle owner)
{
	SDE_PROF_EVENT();
	if (owner.IsValid())
	{
		auto foundStorage = m_components.find(type);
		if (foundStorage != m_components.end())
		{
			static_cast<ComponentType::StorageType*>(foundStorage->second.get())->Destroy(owner);
		}
	}
}

template<class ComponentType>
typename ComponentType::StorageType* World::GetAllComponents()
{
	auto foundStorage = GetStorage(ComponentType::GetType());
	return static_cast<ComponentType::StorageType*>(foundStorage);
}

template<class ComponentType>
ComponentType* World::GetComponent(EntityHandle owner)
{
	SDE_PROF_EVENT();
	if (owner.IsValid())
	{
		auto foundStorage = m_components.find(ComponentType::GetType());
		if (foundStorage != m_components.end())
		{
			return static_cast<ComponentType::StorageType*>(foundStorage->second.get())->Find(owner);
		}
	}
	return nullptr;
}

template<class ComponentType>
ComponentType* World::AddComponent(EntityHandle owner)
{
	SDE_PROF_EVENT();
	if (!owner.IsValid())
	{
		return nullptr;
	}

	auto foundStorage = m_components.find(ComponentType::GetType());
	if (foundStorage != m_components.end())
	{
		return static_cast<ComponentType::StorageType*>(foundStorage->second.get())->Create(owner);
	}
	return nullptr;
}