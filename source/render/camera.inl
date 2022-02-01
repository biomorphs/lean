#include "core/glm_headers.h"

namespace Render
{
	// CTor sets some sane values
	inline Camera::Camera()
		: m_position(0.0f)
		, m_up(0.0f, 1.0f, 0.0f)
		, m_target(0.0f, 0.0f, 1.0f)
		, m_nearPlane(0.1f)
		, m_farPlane(100.0f)
		, m_fov(45.0f)
		, m_aspect(4.0f / 3.0f)
	{
		RebuildViewMatrix();
		RebuildProjectionMatrix();
	}

	inline Camera::~Camera()
	{
	}

	glm::vec3 Camera::WindowPositionToWorldSpace(glm::vec2 p, glm::vec2 windowSize) const
	{
		// place the point on the far plane in clip space (-1 z)?
		glm::vec4 cursorClipspace(-1.0f + (p.x / windowSize.x) * 2.0f, -1.0f + (p.y / windowSize.y) * 2.0f, -1.0f, 1.0f);
		auto inverseView = glm::inverse(m_viewMatrix);
		auto inverseProj = glm::inverse(m_projectionMatrix);
		auto transUnproj = inverseProj * cursorClipspace;
		auto transWs = inverseView * transUnproj;
		transWs = transWs / transWs.w;
		return glm::vec3(transWs);
	}

	inline void Camera::SetProjection(float fov, float aspect, float nearPlane, float farPlane)
	{
		m_fov = fov;
		m_aspect = aspect;
		m_nearPlane = nearPlane;
		m_farPlane = farPlane;
		RebuildProjectionMatrix();
	}

	inline void Camera::SetFOVAndAspectRatio(float fov, float aspect)
	{
		m_fov = fov;
		m_aspect = aspect;
		RebuildProjectionMatrix();
	}

	inline void Camera::SetClipPlanes(float nearPlane, float farPlane)
	{
		m_nearPlane = nearPlane;
		m_farPlane = farPlane;
		RebuildProjectionMatrix();
	}

	inline void Camera::SetPosition(const glm::vec3& pos)
	{
		m_position = pos;
		RebuildViewMatrix();
	}

	inline void Camera::SetUp(const glm::vec3& up)
	{
		m_up = up;
		RebuildViewMatrix();
	}

	inline void Camera::SetTarget(const glm::vec3& target)
	{
		m_target = target;
		RebuildViewMatrix();
	}

	inline void Camera::LookAt(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up)
	{
		m_position = pos;
		m_up = up;
		m_target = target;
		RebuildViewMatrix();
	}

	inline void Camera::LookAt(const glm::vec3& target, const glm::vec3& up)
	{
		m_up = up;
		m_target = target;
		RebuildViewMatrix();
	}

	inline void Camera::LookAt(const glm::vec3& target)
	{
		m_target = target;
		RebuildViewMatrix();
	}

	inline void Camera::RebuildViewMatrix()
	{
		m_viewMatrix = glm::lookAt(m_position, m_target, m_up);
	}

	inline void Camera::RebuildProjectionMatrix()
	{
		m_projectionMatrix = glm::perspective(glm::radians(m_fov), m_aspect, m_nearPlane, m_farPlane);
	}
}