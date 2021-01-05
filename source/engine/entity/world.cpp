#include "world.h"
#include "component.h"
#include "component_storage.h"
#include "engine/script_system.h"
#include <algorithm>

SERIALISE_BEGIN(World)
SERIALISE_PROPERTY("AllEntities", m_allEntities)
SERIALISE_END()

EntityHandle World::AddEntity()
{
	auto newId = m_entityIDCounter++;
	m_allEntities.push_back(newId);
	return EntityHandle(newId);
}

void World::RemoveEntity(EntityHandle h)
{
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

void World::RemoveComponent(EntityHandle owner, Component::Type type)
{
	if (owner.IsValid())
	{
		auto foundStorage = m_components.find(type);
		if (foundStorage != m_components.end())
		{
			foundStorage->second->Destroy(owner);
		}
	}
}

Component* World::GetComponent(EntityHandle owner, Component::Type type)
{
	if (owner.IsValid())
	{
		auto foundStorage = m_components.find(type);
		if (foundStorage != m_components.end())
		{
			return foundStorage->second->Find(owner);
		}
	}
	return nullptr;
}

Component* World::AddComponent(EntityHandle owner, Component::Type type)
{
	if (!owner.IsValid())
	{
		return nullptr;
	}

	auto foundStorage = m_components.find(type);
	if (foundStorage != m_components.end())
	{
		return foundStorage->second->Create(owner);
	}
	return nullptr;
}

std::vector<Component*> World::GetAllComponents(EntityHandle owner)
{
	std::vector<Component*> foundComponents;
	for (const auto& it : m_components)
	{
		Component* found = it.second->Find(owner);
		if (found)
		{
			foundComponents.push_back(found);
		}
	}
	return foundComponents;
}