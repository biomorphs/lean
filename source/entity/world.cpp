#include "world.h"
#include "component.h"
#include "component_storage.h"
#include "engine/script_system.h"
#include "core/profiler.h"
#include <algorithm>
#include <unordered_map>

void World::CollectGarbage()
{
	for (auto entityId : m_pendingDelete)
	{
		for (auto& components : m_components)
		{
			components.second->Destroy(entityId);
		}
	}
	m_pendingDelete.clear();
}

EntityHandle World::AddEntityFromHandle(EntityHandle id)
{
	SDE_PROF_EVENT();
	if (std::find(m_activeEntities.begin(), m_activeEntities.end(), id.GetID()) != m_activeEntities.end())
	{
		SDE_LOG("Entity '%d' already exists!", id.GetID());
		return {};	// an entity already exists with that ID
	}
	if (std::find(m_pendingDelete.begin(), m_pendingDelete.end(), id.GetID()) != m_pendingDelete.end())
	{
		SDE_LOG("Entity '%d' already existed and is being destroyed!", id.GetID());
		return {};	// the old entity didn't clean up fully yet
	}
	// for safety, reset m_entityIDCounter to the highest entity id that exists + 1
	uint32_t maxFoundId = 0;
	for (uint32_t id : m_activeEntities)
	{
		maxFoundId = std::max(id, maxFoundId);
	}
	m_entityIDCounter = maxFoundId + 1;
	m_activeEntities.push_back(id.GetID());
	return id;
}

EntityHandle World::AddEntity()
{
	SDE_PROF_EVENT();
	auto newId = m_entityIDCounter++;
	if (std::find(m_activeEntities.begin(), m_activeEntities.end(), newId) != m_activeEntities.end())
	{
		SDE_LOG("Entity '%d' already exists!", newId);
		return {};	// an entity already exists with that ID
	}
	if (std::find(m_pendingDelete.begin(), m_pendingDelete.end(), newId) != m_pendingDelete.end())
	{
		SDE_LOG("Entity '%d' already existed and is being destroyed!", newId);
		return {};	// the old entity didn't clean up fully yet
	}
	m_activeEntities.push_back(newId);
	return EntityHandle(newId);
}

void World::AddComponent(EntityHandle owner, ComponentType type)
{
	SDE_PROF_EVENT();
	if (owner.IsValid())
	{
		auto foundStorage = m_components.find(type);
		if (foundStorage != m_components.end())
		{
			foundStorage->second->Create(owner);
		}
	}
}

std::vector<ComponentType> World::GetAllComponentTypes()
{
	std::vector<ComponentType> results;
	for (const auto& it : m_components)
	{
		results.push_back(it.first);
	}
	return results;
}

ComponentStorage* World::GetStorage(ComponentType t)
{
	auto foundStorage = m_components.find(t);
	if (foundStorage != m_components.end())
	{
		return foundStorage->second.get();
	}
	return nullptr;
}

std::vector<ComponentType> World::GetOwnedComponentTypes(EntityHandle owner)
{
	std::vector<ComponentType> results;
	for (const auto& component : m_components)
	{
		if (component.second->Contains(owner))
		{
			results.emplace_back(component.first);
		}
	}
	return results;
}

void World::RemoveAllEntities()
{
	SDE_PROF_EVENT();
	m_activeEntities.clear();
	m_pendingDelete.clear();
	for (auto& components : m_components)
	{
		components.second->DestroyAll();
	}
}

void World::RemoveEntity(EntityHandle h)
{
	SDE_PROF_EVENT();
	if (h.IsValid())
	{
		auto foundIt = std::find(m_activeEntities.begin(), m_activeEntities.end(), h.GetID());
		if (foundIt != m_activeEntities.end())
		{
			m_activeEntities.erase(foundIt);
			m_pendingDelete.push_back(h.GetID());
		}
	}
}
