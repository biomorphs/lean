#pragma once
#include "entity_handle.h"

template<class Component>
class ComponentHandle
{
public:
	SERIALISED_CLASS();
	ComponentHandle() = default;
	ComponentHandle(EntityHandle handle);
	Component* GetComponent();
	const EntityHandle& GetEntity() const { return m_entity; }
private:
	EntityHandle m_entity;
	typename Component::StorageType* m_storage = nullptr;
	Component* m_cachedPtr = nullptr;
	uint64_t m_generation = -1;
};

#include "component_handle.inl"