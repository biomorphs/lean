#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"

namespace Engine
{
	class DebugGuiSystem;
	class DebugRender;
}
class World;

class CharacterController
{
public:
	COMPONENT(CharacterController);
	COMPONENT_INSPECTOR();

	float GetRadius() { return m_capsuleRadius; }
	void SetRadius(float r) { m_capsuleRadius = r; }
	float GetHalfHeight() { return m_capsuleHalfHeight; }
	void SetHalfHeight(float h) { m_capsuleHalfHeight = h; }
	float GetVerticalOffset() { return m_verticalOffset; }
	void SetVerticalOffset(float o) { m_verticalOffset = o; }
	glm::vec3 GetCurrentVelocity() { return m_currentVelocity; }
	void SetCurrentVelocity(glm::vec3 v) { m_currentVelocity = v; }

private:
	glm::vec3 m_currentVelocity = { 0.0f,0.0f,0.0f };
	float m_capsuleRadius = 1.0f;
	float m_capsuleHalfHeight = 1.0f;
	float m_verticalOffset = 0.0f;
};