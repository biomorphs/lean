#include "component.h"
#include "core/profiler.h"

template<class ComponentType>
void LinearComponentStorage<ComponentType>::ForEach(std::function<void(Component&,EntityHandle)> fn)
{
	SDE_PROF_EVENT();
	for (int c=0;c<m_components.size();++c)
	{
		fn(m_components[c], m_owners[c]);
	}
}

template<class ComponentType>
Component* LinearComponentStorage<ComponentType>::Find(EntityHandle owner)
{
	auto foundEntity = m_entityToComponent.find(owner);
	if (foundEntity == m_entityToComponent.end())
	{
		return nullptr;
	}
	else
	{
		return &m_components[foundEntity->second];
	}
}

template<class ComponentType>
Component* LinearComponentStorage<ComponentType>::Create(EntityHandle owner)
{
	SDE_PROF_EVENT();
	bool noDuplicate = Find(owner) == nullptr;
	assert(noDuplicate);
	if (noDuplicate)
	{
		m_owners.push_back(owner);
		m_components.push_back(ComponentType());
		uint32_t newIndex = m_components.size() - 1;
		m_entityToComponent.insert({ owner, newIndex });
		return &m_components[newIndex];
	}

	return nullptr;
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::DestroyAll()
{
	SDE_PROF_EVENT();
	m_entityToComponent.clear();
	m_owners.clear();
	m_components.clear();
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::Destroy(EntityHandle owner)
{
	SDE_PROF_EVENT();
	assert(false && "test this");
	auto foundEntity= m_entityToComponent.find(owner);
	if (foundEntity != m_entityToComponent.begin())
	{
		m_owners.erase(m_owners.begin() + foundEntity->second);
		m_components.erase(m_components.begin() + foundEntity->second);

		// we need to repopulate the owner lookup (there is almost certainly a better way)
		m_entityToComponent.clear();
		for (const auto& owner : m_owners)
		{
			auto ownerIndex = &owner - m_owners.data();
		}
	}
}