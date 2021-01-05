#pragma once

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
};

// very basic, slow lookup
template<class ComponentType>
class LinearComponentStorage : public ComponentStorage
{
public:
	virtual Component* Find(EntityHandle owner);
	virtual Component* Create(EntityHandle owner);
	virtual void Destroy(EntityHandle owner);
private:
	std::vector<EntityHandle> m_owners;
	std::vector<ComponentType> m_components;
};

#include "component_storage.inl"