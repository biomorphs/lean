#pragma once

#include "entity/component.h"
#include "entity/component_handle.h"
#include "core/glm_headers.h"

namespace Engine
{
	class DebugGuiSystem;
	class DebugRender;
}

class Transform
{
public:
	COMPONENT(Transform);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui, Engine::DebugRender& render);

	void SetOrientation(glm::quat q) { m_orientation = glm::normalize(q); RebuildMatrix(); }
	void SetRotationDegrees3(float x, float y, float z) { SetOrientation(glm::quat(glm::radians(glm::vec3(x, y, z)))); };
	void SetRotationDegrees(glm::vec3 rotation) { SetOrientation(glm::quat(glm::radians(rotation))); }
	void SetPosition(glm::vec3 p) { m_position = p; RebuildMatrix(); }
	void SetScale(glm::vec3 s) { m_scale = s; RebuildMatrix(); }
	void RotateEuler(glm::vec3 anglesRadians);
	glm::quat GetOrientation() const { return m_orientation; }
	glm::vec3 GetPosition() const { return m_position; }
	glm::vec3 GetRotationRadians() const { return glm::eulerAngles(m_orientation); }
	glm::vec3 GetRotationDegrees() const { return glm::degrees(GetRotationRadians()); }
	glm::vec3 GetScale() const { return m_scale; }
	glm::mat4 GetMatrix() const { return m_matrix; }
	glm::mat4 GetWorldspaceMatrix();
	void SetParent(ComponentHandle<Transform> parent) { m_parent = parent; }
	ComponentHandle<Transform>& GetParent() { return m_parent; }
	const ComponentHandle<Transform>& GetParent() const { return m_parent; }
private:
	void SetPos3(float x, float y, float z) { m_position = { x, y, z }; RebuildMatrix(); }
	void SetScale3(float x, float y, float z) { m_scale = { x, y, z }; RebuildMatrix(); }
	void RebuildMatrix();
	glm::vec3 m_position = { 0.0f,0.0f,0.0f };
	glm::quat m_orientation = glm::identity<glm::quat>();
	glm::vec3 m_scale = { 1.0f,1.0f,1.0f };
	glm::mat4 m_matrix = glm::identity<glm::mat4>();
	ComponentHandle<Transform> m_parent;
};