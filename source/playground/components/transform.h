#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"

class Transform : public Component
{
public:
	COMPONENT(Transform);

	void SetRotation(float x, float y, float z) { SetRotationDegrees({ x,y,z }); };
	void SetRotationDegrees(glm::vec3 rotation) { m_rotation = glm::radians(rotation); }
	void SetPosition(float x, float y, float z) { m_position = { x, y, z }; }
	void SetScale(float x, float y, float z) { m_scale = { x, y, z }; }
	glm::vec3 GetPosition() const { return m_position; }
	glm::vec3 GetRotationRadians() const { return m_rotation; }
	glm::vec3 GetScale() const { return m_scale; }

	glm::mat4 GetMatrix() const {
		glm::mat4 modelMat = glm::translate(glm::identity<glm::mat4>(), m_position);
		modelMat = glm::rotate(modelMat, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMat = glm::rotate(modelMat, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMat = glm::rotate(modelMat, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		return glm::scale(modelMat, m_scale);
	}
private:
	glm::vec3 m_position = { 0.0f,0.0f,0.0f };
	glm::vec3 m_rotation = { 0.0f,0.0f,0.0f };	// RADIANS!!!
	glm::vec3 m_scale = { 1.0f,1.0f,1.0f };
};