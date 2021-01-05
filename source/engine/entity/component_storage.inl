#include "component.h"

// very basic, slow lookup
template<class ComponentType>
Component* LinearComponentStorage<ComponentType>::Find(EntityHandle owner)
{
	auto foundEntity = std::find(m_owners.begin(), m_owners.end(), owner);
	if (foundEntity == m_owners.end())
	{
		return nullptr;
	}
	else
	{
		return &m_components[foundEntity - m_owners.begin()];
	}
}

template<class ComponentType>
Component* LinearComponentStorage<ComponentType>::Create(EntityHandle owner)
{
	bool noDuplicate = Find(owner) == nullptr;
	assert(noDuplicate);
	if (noDuplicate)
	{
		m_owners.push_back(owner);
		m_components.push_back(ComponentType());
		return &m_components[m_components.size() - 1];
	}

	return nullptr;
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::Destroy(EntityHandle owner)
{
	auto foundEntity=std::find(m_owners.begin(), m_owners.end(), owner);
	if (foundEntity != m_owners.begin())
	{
		auto indexToRemove = foundEntity - m_owners.begin();
		m_owners.erase(foundEntity);
		m_components.erase(m_components.begin() + indexToRemove);
	}
}