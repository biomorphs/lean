#pragma once
#include "core/glm_headers.h"

namespace Render
{
	class Camera;
}

namespace Engine
{
	struct ControllerRawState;
	struct MouseRawState;
	struct KeyboardState;

	class DebugCamera
	{
	public:
		DebugCamera();
		virtual ~DebugCamera();
		void ApplyToCamera(Render::Camera& target);
		void Update(const Engine::ControllerRawState& controllerState, double timeDelta);
		void Update(const Engine::MouseRawState& mouseState, double timeDelta);
		void Update(const Engine::KeyboardState& kbState, double timeDelta);
		inline void SetPosition(const glm::vec3& pos) { m_position = pos; }
		inline void SetYaw(float y) { m_yaw = y; }
		inline void SetPitch(float p) { m_pitch = p; }
		inline glm::vec3 GetPosition() const { return m_position; }
	private:
		glm::vec3 m_position;
		glm::vec3 m_lookDirection;
		glm::vec3 m_right;
		float m_pitch;
		float m_yaw;
	};
}