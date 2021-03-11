#include "world.h"
#include "component.h"
#include "component_storage.h"
#include "engine/script_system.h"
#include "core/profiler.h"
#include <algorithm>

SERIALISE_BEGIN(World)
SERIALISE_PROPERTY("AllEntities", m_allEntities)
//SERIALISE_PROPERTY("ComponentStorage", m_components)
SERIALISE_END()

EntityHandle World::AddEntity()
{
	SDE_PROF_EVENT();
	auto newId = m_entityIDCounter++;
	m_allEntities.push_back(newId);
	return EntityHandle(newId);
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
	m_allEntities.clear();
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
		auto foundIt = std::find(m_allEntities.begin(), m_allEntities.end(), h.GetID());
		if (foundIt != m_allEntities.end())
		{
			for (auto& components : m_components)
			{
				components.second->Destroy(*foundIt);
			}

			m_allEntities.erase(foundIt);
		}
	}
}
