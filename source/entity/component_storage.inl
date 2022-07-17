#include "core/profiler.h"
#include "core/log.h"
#include "core/thread.h"
#include <atomic>

constexpr int c_maxComponents = 1024 * 64;

template<class ComponentType>
uint64_t LinearComponentStorage<ComponentType>::GetActiveCount()
{
	return m_components.size();
}

template<class ComponentType>
uint64_t LinearComponentStorage<ComponentType>::GetActiveSizeBytes()
{
	uint64_t totalSize = m_owners.size() * sizeof(EntityHandle);
	totalSize += m_components.size() * sizeof(ComponentType);
	totalSize += m_entityToComponent.calcNumBytesInfo(m_entityToComponent.size());
	return totalSize;
}

template<class ComponentType>
uint64_t  LinearComponentStorage<ComponentType>::GetTotalSizeBytes()
{
	uint64_t totalSize = m_owners.capacity() * sizeof(EntityHandle);
	totalSize += m_components.capacity() * sizeof(ComponentType);
	totalSize += m_entityToComponent.calcNumBytesTotal(m_entityToComponent.size());
	return totalSize;
}

template<class ComponentType>
LinearComponentStorage<ComponentType>::LinearComponentStorage()
{
	m_owners.reserve(c_maxComponents);
	m_components.reserve(c_maxComponents);
}

template<class ComponentType>
void LinearComponentStorage<ComponentType>::ForEachAsync(std::function<void(ComponentType&, EntityHandle)> fn, Engine::JobSystem& js, int32_t componentsPerJob)
{
	SDE_PROF_EVENT();

	// We need to ensure the integrity of the list during iterations
	// You can safely add components during iteration, but you CANNOT delete them!
	++m_iterationDepth;
	assert(m_owners.size() == m_components.size() && m_entityToComponent.size() == m_owners.size());

	// more safety nets, ensure storage doesn't move
	void* storagePtr = m_components.data();

	std::atomic<int> jobsRemaining = 0;
	{
		SDE_PROF_EVENT("PushJobs");
		const int32_t currentActiveComponents = m_components.size();
		for (int32_t i = 0; i < m_components.size(); i += componentsPerJob)
		{
			const int startIndex = i;
			const int endIndex = std::min(i + componentsPerJob, currentActiveComponents);
			auto runJob = [this, startIndex, endIndex, &jobsRemaining, &fn](void*) {
				for (int c = startIndex; c < endIndex; ++c)
				{
					fn(m_components[c], m_owners[c]);
				}
				jobsRemaining--;
			};
			jobsRemaining++;
			js.PushJob(runJob);
		}
	}

	// wait for the results
	{
		SDE_PROF_STALL("WaitForResults");
		while (jobsRemaining > 0)
		{
			js.ProcessJobThisThread();
			Core::Thread::Sleep(0);
		}
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
void LinearComponentStorage<ComponentType>::Serialise(EntityHandle owner, nlohmann::json& json, Engine::SerialiseType op)
{
	ComponentType* foundComponent = Find(owner);
	if (op == Engine::SerialiseType::Write)
	{
		if (foundComponent)
		{
			Engine::ToJson(*foundComponent, json);
		}
	}
	else
	{
		if (!foundComponent)
		{
			Create(owner);
			foundComponent = Find(owner);
		}
		Engine::FromJson(*foundComponent, json);
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

		size_t oldCapacity = m_components.capacity();
		m_owners.push_back(owner);
		m_components.emplace_back(std::move(ComponentType()));
		uint32_t newIndex = m_components.size() - 1;
		m_entityToComponent.insert({ owner.GetID(), newIndex });
		++m_generation;
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
	++m_generation;
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

		std::iter_swap(m_owners.begin() + currentIndex, m_owners.end() - 1);
		m_owners.pop_back();
		std::iter_swap(m_components.begin() + currentIndex, m_components.end() - 1);
		m_components.pop_back();

		// we may want to be more fancy and only invalidate particular handles, but for now this works
		++m_generation;

		if (currentIndex < m_owners.size())	// why? investigate
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