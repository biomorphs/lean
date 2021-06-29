#pragma once
#include "robin_hood.h"

class EntityHandle;

namespace Engine
{
	class JobSystem;
}

// generic storage interface
class ComponentStorage
{
public:
	ComponentStorage() = default;
	virtual ~ComponentStorage() = default;
	virtual SERIALISED_CLASS() {}

	virtual void Create(EntityHandle owner) = 0;
	virtual bool Contains(EntityHandle owner) = 0;
	virtual void Destroy(EntityHandle owner) = 0;
	virtual void DestroyAll() = 0;
};

// a fixed size array of components with fast(ish) lookups
template<class ComponentType>
class LinearComponentStorage : public ComponentStorage
{
public:
	SERIALISED_CLASS();
	LinearComponentStorage();
	ComponentType* Find(EntityHandle owner);
	void ForEach(std::function<void(ComponentType&, EntityHandle)> fn);
	void ForEachAsync(std::function<void(ComponentType&, EntityHandle)> fn, Engine::JobSystem& js, int32_t componentsPerJob);

	virtual void Create(EntityHandle owner);
	virtual bool Contains(EntityHandle owner) { return Find(owner) != nullptr; }
	virtual void Destroy(EntityHandle owner);
	virtual void DestroyAll();
private:
	robin_hood::unordered_map<uint32_t, uint32_t> m_entityToComponent;
	std::vector<EntityHandle> m_owners;
	std::vector<ComponentType> m_components;
	int32_t m_iterationDepth = 0;	// this is a safety net to catch if we delete during iteration
};

#include "component_storage.inl"