#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"

class Transform
{
public:
	COMPONENT(Transform);

	void SetRotation(float x, float y, float z) { SetRotationDegrees({ x,y,z }); RebuildMatrix(); };
	void SetRotationDegrees(glm::vec3 rotation) { m_rotation = glm::radians(rotation); RebuildMatrix(); }
	void SetPosition(glm::vec3 p) { m_position = p; RebuildMatrix(); }
	void SetScale(glm::vec3 s) { m_scale = s; RebuildMatrix(); }
	glm::vec3 GetPosition() const { return m_position; }
	glm::vec3 GetRotationRadians() const { return m_rotation; }
	glm::vec3 GetRotationDegrees() const { return glm::degrees(m_rotation); }
	glm::vec3 GetScale() const { return m_scale; }
	glm::mat4 GetMatrix() const { return m_matrix; }
private:
	void SetPos3(float x, float y, float z) { m_position = { x, y, z }; RebuildMatrix(); }
	void SetScale3(float x, float y, float z) { m_scale = { x, y, z }; RebuildMatrix(); }
	void RebuildMatrix();
	glm::vec3 m_position = { 0.0f,0.0f,0.0f };
	glm::vec3 m_rotation = { 0.0f,0.0f,0.0f };	// RADIANS!!!
	glm::vec3 m_scale = { 1.0f,1.0f,1.0f };
	glm::mat4 m_matrix = glm::identity<glm::mat4>();
};