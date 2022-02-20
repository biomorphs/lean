#pragma once
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include <vector>

class TransformWidget
{
public:
	void Update(const std::vector<EntityHandle>& entities);
private:
	void Reset(const std::vector<EntityHandle>& entities);
	struct TrackedEntity
	{
		EntityHandle m_entity;
		glm::mat4 m_relativeTransform;	// transform relative to the widget
	};
	std::vector<EntityHandle> m_currentEntities;
	std::vector<TrackedEntity> m_trackedEntities;
	glm::mat4 m_widgetTransform;
};