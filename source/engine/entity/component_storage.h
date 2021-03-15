#pragma once
#include <unordered_map>
#include <map>
#include "robin_hood.h"

class EntityHandle;

// generic storage interface
class ComponentStorage
{
public:
	ComponentStorage() = default;
	virtual ~ComponentStorage() = default;
	SERIALISED_CLASS();

	virtual void Create(EntityHandle owner) = 0;
	virtual bool Contains(EntityHandle owner) = 0;
	virtual void Destroy(EntityHandle owner) = 0;
	virtual void DestroyAll() = 0;
};

// very basic, slow lookup
template<class ComponentType>
class LinearComponentStorage : public ComponentStorage
{
public:
	ComponentType* Find(EntityHandle owner);
	void ForEach(std::function<void(ComponentType&, EntityHandle)> fn);

	virtual void Create(EntityHandle owner);
	virtual bool Contains(EntityHandle owner) { return Find(owner) != nullptr; }
	virtual void Destroy(EntityHandle owner);
	virtual void DestroyAll();
private:
	robin_hood::unordered_map<uint32_t, uint32_t> m_entityToComponent;
	std::vector<EntityHandle> m_owners;
	std::vector<ComponentType> m_components;
};

#include "component_storage.inl"