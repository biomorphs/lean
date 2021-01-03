#include "arcball_camera.h"
#include "mouse_state.h"

namespace Engine
{
	void ArcballCamera::Update(const Engine::MouseRawState& ms, float deltaTime)
	{
		if (ms.m_buttonState & Engine::LeftButton)
		{
			if (!m_enabled)
			{
				m_anchorPoint = m_currentPoint = { ms.m_cursorX, ms.m_cursorY };
				m_enabled = true;
			}
		}
		else
		{
			m_enabled = false;
		}
		m_currentPoint = { ms.m_cursorX, ms.m_cursorY };

		if (ms.m_wheelScroll != 0)
		{
			static float c_moveSpeed = 20.0f;
			float movement = (float)-ms.m_wheelScroll * deltaTime * c_moveSpeed;
			auto pointToTargetDir = glm::normalize(m_target - m_position);
			float currentDistance = glm::length(m_target - m_position);
			float newDistance = currentDistance + movement;
			if (newDistance >= 0.001f)
			{
				m_position = m_target - (pointToTargetDir * newDistance);
			}
		}

		float distance = glm::length(glm::vec2(m_currentPoint) - glm::vec2(m_anchorPoint));
		if (m_enabled && distance > 0.0f)
		{
			glm::vec4 position(m_position.x, m_position.y, m_position.z, 1);
			glm::vec4 anchorPoint(m_target.x, m_target.y, m_target.z, 1);
			float deltaAngleX = (2.0f * glm::pi<float>() / m_windowSize.x); // a movement from left to right = 2*PI = 360 deg
			float deltaAngleY = (glm::pi<float>() / m_windowSize.y);  // a movement from top to bottom = PI = 180 deg
			float xAngle = (m_anchorPoint.x - m_currentPoint.x) * deltaAngleX * deltaTime;
			float yAngle = (m_anchorPoint.y - m_currentPoint.y) * deltaAngleY * deltaTime;

			// handle dir=up
			float cosTheta = glm::dot(m_target - m_position, m_upVector);
			if (cosTheta * sin(deltaAngleY) > 0.99f)
			{
				deltaAngleY = 0.0f;
			}

			glm::mat4x4 rotationMatrixX(1.0f);
			rotationMatrixX = glm::rotate(rotationMatrixX, xAngle, m_upVector);
			position = (rotationMatrixX * (position - anchorPoint)) + anchorPoint;

			glm::mat4x4 rotationMatrixY(1.0f);
			glm::vec3 rightVec = glm::cross(glm::normalize(m_target - m_position), m_upVector);
			rotationMatrixY = glm::rotate(rotationMatrixY, yAngle, rightVec);
			glm::vec4 finalPosition = (rotationMatrixY * (position - anchorPoint)) + anchorPoint;

			m_position = glm::vec3(finalPosition);
		}
	}

}