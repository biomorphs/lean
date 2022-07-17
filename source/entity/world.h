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

	EntityHandle AddEntityFromHandle(EntityHandle id);	// only the tools and serialiser are allowed to use this! return valid handle if success
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
	int GetPendingDeleteCount() { return m_pendingDelete.size(); }

	template<class Cmp1, class Cmp2>
	class EntityIterator
	{
	public:
		EntityIterator(World* w) : m_world(w) {}
		void ForEach(std::function<void(Cmp1&, Cmp2&, EntityHandle)> fn);
	private:
		World* m_world;
		uint64_t m_lastGeneration1 = 0;
		uint64_t m_lastGeneration2 = 0;
		std::vector<Cmp1*> m_cmp1;	// only touch these if you validate the generation first!
		std::vector<Cmp2*> m_cmp2;	// only touch these if you validate the generation first!
		std::vector<EntityHandle> m_entities;
	};

	template<class Cmp1, class Cmp2>
	EntityIterator<Cmp1, Cmp2> MakeIterator();

private:
	uint32_t m_entityIDCounter;
	std::vector<uint32_t> m_activeEntities;	// all active entity IDs
	std::vector<uint32_t> m_pendingDelete;	// all entities to be deleted
	robin_hood::unordered_map<ComponentType, std::unique_ptr<ComponentStorage>> m_components;	// all active component data
};

template<class Cmp1, class Cmp2>
void World::EntityIterator<Cmp1,Cmp2>::ForEach(std::function<void(Cmp1&, Cmp2&, EntityHandle)> fn)
{
	SDE_PROF_EVENT();

	const uint64_t currentGen1 = m_world->GetStorage(Cmp1::GetType())->GetGeneration();
	const uint64_t currentGen2 = m_world->GetStorage(Cmp2::GetType())->GetGeneration();
	const bool listsDirty = m_lastGeneration1 != currentGen1 || m_lastGeneration2 != currentGen2;

	// Slow path
	if (listsDirty)
	{
		m_cmp1.clear();
		m_cmp2.clear();
		m_entities.clear();
		m_world->ForEachComponent<Cmp1>([this, &fn](Cmp1& c1, EntityHandle h) {
			Cmp2* c2 = m_world->GetComponent<Cmp2>(h);
			if (c2 != nullptr)
			{
				m_cmp1.push_back(&c1);
				m_cmp2.push_back(c2);
				m_entities.push_back(h);
			}
		});
		m_lastGeneration1 = currentGen1;
		m_lastGeneration2 = currentGen2;
	}

	const int count = m_entities.size();
	for (int index = 0; index < count; ++index)
	{
		fn(*m_cmp1[index], *m_cmp2[index], m_entities[index]);
	}
}

template<class Cmp1, class Cmp2>
World::EntityIterator<Cmp1, Cmp2> World::MakeIterator()
{
	EntityIterator<Cmp1, Cmp2> it(this);
	return it;
}

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
	SDE_PROF_EVENT();
	auto foundStorage = m_components.find(ComponentType::GetType());
	if (foundStorage != m_components.end())
	{
		return static_cast<ComponentType::StorageType*>(foundStorage->second.get())->ForEachAsync(fn, jsys, componentsPerJob);
	}
}

template<class ComponentType>
void World::ForEachComponent(std::function<void(ComponentType&, EntityHandle)> fn)
{
	SDE_PROF_EVENT();
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