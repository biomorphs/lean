#include "core/profiler.h"
#include "core/log.h"

constexpr int c_maxComponents = 1024 * 64;

template<class ComponentType>
LinearComponentStorage<ComponentType>::LinearComponentStorage()
{
	m_owners.reserve(c_maxComponents);
	m_components.reserve(c_maxComponents);
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::ForEach(std::function<void(ComponentType&,EntityHandle)> fn)
{
	SDE_PROF_EVENT();
	
	// We need to ensure the integrity of the list during iterations
	// You can safely add components during iteration, but you CANNOT delete them!
	++m_iterationDepth;
	assert(m_owners.size() == m_components.size() && m_entityToComponent.size() == m_owners.size());

	// more safety nets, ensure storage doesn't move
	void* storagePtr = m_components.data();

	auto currentActiveComponents = m_components.size();
	for (int c=0;c<currentActiveComponents;++c)
	{
		fn(m_components[c], m_owners[c]);
	}

	if (storagePtr != m_components.data())
	{
		SDE_LOG("NO! Storage ptr was changed during iteration!");
		assert(!"NO! Storage ptr was changed during iteration!");
		*((int*)0x0) = 3;	// force crash
	}

	--m_iterationDepth;
}

template<class ComponentType>
ComponentType* LinearComponentStorage<ComponentType>::Find(EntityHandle owner)
{
	auto foundEntity = m_entityToComponent.find(owner.GetID());
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
void LinearComponentStorage<ComponentType>::Create(EntityHandle owner)
{
	SDE_PROF_EVENT();

	bool noDuplicate = Find(owner) == nullptr;
	assert(noDuplicate);
	if (noDuplicate)
	{
		if (m_components.size() >= c_maxComponents)
		{
			SDE_LOG("NO! Maximum components reached!");
			assert(!"NO! Maximum components reached!");
			*((int*)0x0) = 3;	// force crash
		}

		m_owners.push_back(owner);
		m_components.emplace_back(std::move(ComponentType()));
		uint32_t newIndex = m_components.size() - 1;
		m_entityToComponent.insert({ owner.GetID(), newIndex });
	}
	assert(m_owners.size() == m_components.size() && m_entityToComponent.size() == m_owners.size());
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::DestroyAll()
{
	SDE_PROF_EVENT();

	if (m_iterationDepth > 0)
	{
		SDE_LOG("NO! You cannot delete during iteration!");
		assert(!"NO! You cannot delete during iteration!");
		*((int*)0x0) = 3;	// force crash
	}

	m_entityToComponent.clear();
	m_owners.clear();
	m_components.clear();
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::Destroy(EntityHandle owner)
{
	SDE_PROF_EVENT();

	if (m_iterationDepth > 0)
	{
		SDE_LOG("NO! You cannot delete during iteration!");
		assert(!"NO! You cannot delete during iteration!");
		*((int*)0x0) = 3;	// force crash
	}

	auto foundEntity = m_entityToComponent.find(owner.GetID());
	if (foundEntity != m_entityToComponent.end())
	{
		auto currentIndex = foundEntity->second;

		// swap and pop for fast deletions (safe since nobody is allowed to cache component ptrs anyway)
		std::iter_swap(m_owners.begin() + currentIndex, m_owners.end() - 1);
		m_owners.pop_back();
		std::iter_swap(m_components.begin() + currentIndex, m_components.end() - 1);
		m_components.pop_back();

		if (currentIndex < m_owners.size())
		{
			// fix up the lookup for the component we just moved
			auto ownerEntity = m_owners[currentIndex];
			m_entityToComponent[ownerEntity.GetID()] = currentIndex;
		}

		// remove the old lookup
		m_entityToComponent.erase(foundEntity);
	}

	assert(m_owners.size() == m_components.size() && m_entityToComponent.size() == m_owners.size());
}