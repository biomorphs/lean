#pragma once
#include <functional>
#include "engine/system_manager.h"
#include "entity/entity_system.h"
#include "entity_handle.h"

// Always wrap component setters with this!
// Since we want undo/redo where components move around in memory, we need to 
// get the component each time, which results in this dance
template<class Component, class ValueType>
auto InspectFn(const EntityHandle& e, void(Component::* setFn)(ValueType))
{
	auto wrappedFn = [e, setFn](ValueType v)
	{
		// always get the component ptr from the world (We cannot cache them!)
		auto world = Engine::GetSystem<EntitySystem>("Entities")->GetWorld();
		Component* cmp = world->GetComponent<Component>(e);
		if (cmp)
		{
			(cmp->*setFn)(v);
		}
	};
	return wrappedFn;
}

class ComponentInspector
{
public:
	virtual bool Inspect(const char* label, EntityHandle current, std::function<void(EntityHandle)> setFn, std::function<bool(const EntityHandle&)> filter) = 0;
	virtual bool InspectEnum(const char* label, int currentValue, std::function<void(int)> setFn, const char* enumStrs[], int enumStrCount) = 0;
	virtual bool Inspect(const char* label, bool currentValue, std::function<void(bool)> setFn) = 0;
	virtual bool Inspect(const char* label, int currentValue, std::function<void(int)> setFn, int step=1, int minv=INT_MIN, int maxv=INT_MAX) = 0;
	virtual bool Inspect(const char* label, float currentValue, std::function<void(float)> setFn, float step = 1.0f, float minv = -FLT_MAX, float maxv = FLT_MAX) = 0;
	virtual bool Inspect(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn, float step = 1.0f, float minv = -FLT_MAX, float maxv = FLT_MAX) = 0;
	virtual bool InspectColour(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn) = 0;
};