#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"

class Transform : public Component
{
public:
	COMPONENT(Transform);

	void SetPosition(float x, float y, float z) { m_position = { x, y, z }; }
	void SetScale(float x, float y, float z) { m_scale = { x, y, z }; }
	glm::vec3 GetPosition() const { return m_position; }

	glm::mat4 GetMatrix() const {
		return glm::scale(glm::translate(glm::identity<glm::mat4>(), m_position), m_scale);
	}
private:
	glm::vec3 m_position = { 0.0f,0.0f,0.0f };
	glm::vec3 m_scale = { 1.0f,1.0f,1.0f };
};