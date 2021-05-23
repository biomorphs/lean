#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"

class Transform
{
public:
	COMPONENT(Transform);

	void SetOrientation(glm::quat q) { m_orientation = glm::normalize(q); RebuildMatrix(); }
	void SetRotation(float x, float y, float z) { SetOrientation(glm::quat(glm::vec3(x, y, z))); };
	void SetRotationDegrees(glm::vec3 rotation) { SetOrientation(glm::quat(glm::radians(rotation))); }
	void SetPosition(glm::vec3 p) { m_position = p; RebuildMatrix(); }
	void SetScale(glm::vec3 s) { m_scale = s; RebuildMatrix(); }
	glm::quat GetOrientation() const { return m_orientation; }
	glm::vec3 GetPosition() const { return m_position; }
	glm::vec3 GetRotationRadians() const { return glm::eulerAngles(m_orientation); }
	glm::vec3 GetRotationDegrees() const { return glm::degrees(GetRotationRadians()); }
	glm::vec3 GetScale() const { return m_scale; }
	glm::mat4 GetMatrix() const { return m_matrix; }
private:
	void SetPos3(float x, float y, float z) { m_position = { x, y, z }; RebuildMatrix(); }
	void SetScale3(float x, float y, float z) { m_scale = { x, y, z }; RebuildMatrix(); }
	void RebuildMatrix();
	glm::vec3 m_position = { 0.0f,0.0f,0.0f };
	glm::quat m_orientation = glm::identity<glm::quat>();
	glm::vec3 m_scale = { 1.0f,1.0f,1.0f };
	glm::mat4 m_matrix = glm::identity<glm::mat4>();
};