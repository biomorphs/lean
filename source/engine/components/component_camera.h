#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"

class Camera
{
public:
	COMPONENT(Camera);

	inline void SetNearPlane(float p) { m_nearPlane = p; }
	inline float GetNearPlane() const { return m_nearPlane; }
	inline void SetFarPlane(float p) { m_farPlane = p; }
	inline float GetFarPlane() const { return m_farPlane; }
	inline void SetFOV(float f) { m_fov = f; }
	inline float GetFOV() const { return m_fov; }

private:
	float m_nearPlane = 0.1f;
	float m_farPlane = 5000.0f;
	float m_fov = 70.0f;
};