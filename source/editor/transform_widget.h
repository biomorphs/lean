#pragma once
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include <vector>

class TransformWidget
{
public:
	void Update(const std::vector<EntityHandle>& entities);
private:
	void RestoreEntityTransforms();
	void Reset(const std::vector<EntityHandle>& entities);
	void CommitTranslation(glm::vec3 translation);
	struct TrackedEntity
	{
		EntityHandle m_entity;
		glm::vec3 m_originalPosition;
	};
	std::vector<EntityHandle> m_currentEntities;
	std::vector<TrackedEntity> m_trackedEntities;
	glm::mat4 m_widgetTransform;	// current transform
	glm::mat4 m_mouseDragStartTransform;	// transform when mouse started being dragged
	glm::vec3 m_mouseClickPosOnAxis;		// position on the original axis where drag started
	bool m_mouseBtnDownLastFrame = false;
};