#pragma once
#include "engine/job_system.h"
#include "entity_handle.h"
#include "component.h"
#include "component_storage.h"
#include "robin_hood.h"
#include <string>

namespace Engine
{
	class JobSystem;
}

class EntityHandle;
class World
{
public:
	World() = default;
	~World() = default;

	EntityHandle AddEntity();
	void RemoveEntity(EntityHandle h);	// defers actual deletion until CollectGarbage() called
	const std::vector<uint32_t>& AllEntities() const { return m_activeEntities; }
	void RemoveAllEntities();		// instant, use with caution

	template<class ComponentType>
	void RegisterComponentType();

	template<class ComponentType>
	void ForEachComponent(std::function<void(ComponentType&, EntityHandle)> fn);

	template<class ComponentType>
	void ForEachComponentAsync(std::function<void(ComponentType&, EntityHandle)> fn, Engine::JobSystem& jsys, int32_t componentsPerJob=100);

	void AddComponent(EntityHandle owner, ComponentType type);
	std::vector<ComponentType> GetAllComponentTypes();

	template<class ComponentType>
	ComponentType* GetComponent(EntityHandle owner);

	template<class ComponentType>
	typename ComponentType::StorageType* GetAllComponents();

	ComponentStorage* GetStorage(ComponentType t);

	std::vector<ComponentType> GetOwnedComponentTypes(EntityHandle owner);

	template<class ComponentType>
	void RemoveComponent(EntityHandle owner);

	void CollectGarbage();					// destroy all entities pending deletion

private:
	uint32_t m_entityIDCounter;
	std::vector<uint32_t> m_activeEntities;	// all active entity IDs
	std::vector<uint32_t> m_pendingDelete;	// all entities to be deleted
	robin_hood::unordered_map<ComponentType, std::unique_ptr<ComponentStorage>> m_components;	// all active component data
};

template<class ComponentType>
void World::RegisterComponentType()
{
	assert(m_components.find(ComponentType::GetType()) == m_components.end());
	auto storage = std::make_unique<typename ComponentType::StorageType>();
	m_components[ComponentType::GetType()] = std::move(storage);
}

template<class ComponentType>
void World::ForEachComponentAsync(std::function<void(ComponentType&, EntityHandle)> fn, Engine::JobSystem& jsys, int32_t componentsPerJob)
{
	auto foundStorage = m_components.find(ComponentType::GetType());
	if (foundStorage != m_components.end())
	{
		return static_cast<ComponentType::StorageType*>(foundStorage->second.get())->ForEachAsync(fn, jsys, componentsPerJob);
	}
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