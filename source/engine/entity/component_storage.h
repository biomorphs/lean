#pragma once
#include <unordered_map>
#include <map>
#include "robin_hood.h"

class Component;
class EntityHandle;

// generic storage interface
class ComponentStorage
{
public:
	ComponentStorage() = default;
	virtual ~ComponentStorage() = default;
	SERIALISED_CLASS();

	virtual Component* Find(EntityHandle owner) = 0;
	virtual Component* Create(EntityHandle owner) = 0;
	virtual void Destroy(EntityHandle owner) = 0;
	virtual void DestroyAll() = 0;

	// iteration
	virtual void ForEach(std::function<void(Component&, EntityHandle)> fn) = 0;
};

// very basic, slow lookup
template<class ComponentType>
class LinearComponentStorage : public ComponentStorage
{
public:
	virtual Component* Find(EntityHandle owner);
	virtual Component* Create(EntityHandle owner);
	virtual void Destroy(EntityHandle owner);
	virtual void DestroyAll();
	virtual void ForEach(std::function<void(Component&,EntityHandle)> fn);
private:
	robin_hood::unordered_map<uint32_t, uint32_t> m_entityToComponent;
	std::vector<EntityHandle> m_owners;
	std::vector<ComponentType> m_components;
};

#include "component_storage.inl"