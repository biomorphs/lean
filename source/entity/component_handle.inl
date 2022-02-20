#include "world.h"
#include "engine/system_manager.h"
#include "entity/entity_system.h"
#include "world.h"

template<class Component>
void ComponentHandle<Component>::Serialise(nlohmann::json& json, Engine::SerialiseType op)
{
	if (op == Engine::SerialiseType::Write)
	{
		Engine::ToJson(m_entity, json);
	}
	else
	{
		Engine::FromJson(m_entity, json);
	}
}

template<class Component>
ComponentHandle<Component>::ComponentHandle(EntityHandle handle)
	: m_entity(handle)
{
}

template<class Component>
Component* ComponentHandle<Component>::GetComponent()
{
	Component* ptr = nullptr;
	if (m_entity.GetID() != -1)
	{
		if (m_storage == nullptr)
		{
			auto world = Engine::GetSystem<EntitySystem>("Entities")->GetWorld();
			m_storage = static_cast<Component::StorageType*>(world->GetStorage(Component::GetType()));
		}

		if (m_cachedPtr && m_storage->GetGeneration() == m_generation)
		{
			ptr = m_cachedPtr;
		}
		else
		{
			ptr = m_cachedPtr = m_storage->Find(m_entity);
			m_generation = m_storage->GetGeneration();
		}
	}
	return ptr;
}