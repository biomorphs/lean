#pragma once
#include "core/glm_headers.h"

namespace Engine
{
	struct MouseRawState;

	class ArcballCamera
	{
	public:
		ArcballCamera() = default;
		void Update(const Engine::MouseRawState&, float dt);
		void SetWindowSize(glm::ivec2 windowSize) { m_windowSize = windowSize; }
		inline void SetPosition(const glm::vec3& pos) { m_position = pos; }
		inline void SetTarget(const glm::vec3& t) { m_target = t; }
		inline void SetUp(const glm::vec3& u) { m_upVector = u; }
		
		glm::vec3 GetPosition() { return m_position; }
		glm::vec3 GetTarget() { return m_target; }
		glm::vec3 GetUp() { return m_upVector; }
	private:
		bool m_enabled = false;
		glm::ivec2 m_anchorPoint = { 0,0 };	// first 'click' point
		glm::ivec2 m_currentPoint = { 0,0 };
		glm::ivec2 m_windowSize = { 0,0 };
		glm::vec3 m_position = { 0.0f,0.0f,-1.0f };
		glm::vec3 m_target = { 0.0,0.0,0.0f };
		glm::vec3 m_upVector = { 0.0f,1.0f,0.0f };
	};
}