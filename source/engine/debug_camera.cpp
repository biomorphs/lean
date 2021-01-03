#include "debug_camera.h"
#include "engine/controller_state.h"
#include "engine/mouse_state.h"
#include "engine/keyboard_state.h"
#include "render/camera.h"
#include "core/profiler.h"

namespace Engine
{
	DebugCamera::DebugCamera()
		: m_position(0.0f, 0.0f, 0.0f)
		, m_pitch(0.0f)
		, m_yaw(0.0f)
	{

	}

	DebugCamera::~DebugCamera()
	{

	}

	void DebugCamera::ApplyToCamera(Render::Camera& target)
	{
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		target.LookAt(m_position, m_position + m_lookDirection, up);
	}

	void DebugCamera::Update(const Engine::MouseRawState& mouse, double timeDelta)
	{
		SDE_PROF_EVENT();

		const auto mousePosition = glm::ivec2(mouse.m_cursorX, mouse.m_cursorY);
		const float timeDeltaF = (float)timeDelta;
		static float s_yawRotSpeed = 0.01f;
		static float s_pitchRotSpeed = 0.01f;
		static bool enabled = false;
		static glm::ivec2 lastClickPos = { 0,0 };
		
		if (mouse.m_buttonState & Engine::MouseButtons::LeftButton)
		{
			if (!enabled)
			{
				lastClickPos = mousePosition;
				enabled = true;
			}
			else
			{
				glm::ivec2 movement = mousePosition - lastClickPos;
				glm::vec2 movementAtSpeed = glm::vec2(movement);
				const float yawRotation = -movementAtSpeed.x * s_yawRotSpeed * timeDeltaF;
				m_yaw += yawRotation;

				const float pitchRotation = -movementAtSpeed.y * s_pitchRotSpeed * timeDeltaF;
				m_pitch += pitchRotation;
			}
		}
		else
		{
			enabled = false;
		}
	}

	void DebugCamera::Update(const Engine::KeyboardState& kbState, double timeDelta)
	{
		const float timeDeltaF = (float)timeDelta;
		static float s_forwardSpeed = 10.0f;
		static float s_strafeSpeed = 10.0f;
		float forwardAmount = 0.0f;
		float strafeAmount = 0.0f;
		float speedMul = 1.0f;
		if (kbState.m_keyPressed[Engine::KEY_LSHIFT] || kbState.m_keyPressed[Engine::KEY_RSHIFT])
		{ 
			speedMul = 10.0f;
		}
		if (kbState.m_keyPressed[Engine::KEY_w])
		{
			forwardAmount = 1.0f;
		}
		if (kbState.m_keyPressed[Engine::KEY_s])
		{
			forwardAmount = -1.0f;
		}
		if (kbState.m_keyPressed[Engine::KEY_d])
		{
			strafeAmount = 1.0f;
		}
		if (kbState.m_keyPressed[Engine::KEY_a])
		{
			strafeAmount = -1.0f;
		}

		// move forward
		const float forward = speedMul * forwardAmount * s_forwardSpeed * timeDeltaF;
		m_position += m_lookDirection * forward;

		// strafe
		const float strafe = speedMul * strafeAmount * s_strafeSpeed * timeDeltaF;
		m_position += m_right * strafe;
	}

	void DebugCamera::Update(const Engine::ControllerRawState& controllerState, double timeDelta)
	{
		SDE_PROF_EVENT();

		static float s_yawRotSpeed = 2.0f;
		static float s_pitchRotSpeed = 2.0f;
		static float s_forwardSpeed = 1.0f;
		static float s_strafeSpeed = 1.0f;
		static float s_speedMultiplier = 10.0f;
		static float s_highSpeedMultiplier = 50.0f;

		const float timeDeltaF = (float)timeDelta;
		const float xAxisRight = controllerState.m_rightStickAxes[0];
		const float yAxisRight = controllerState.m_rightStickAxes[1];
		const float xAxisLeft = controllerState.m_leftStickAxes[0];
		const float yAxisLeft = controllerState.m_leftStickAxes[1];

		float moveSpeedMulti = 1.0f + (controllerState.m_rightTrigger * s_speedMultiplier) + ((controllerState.m_leftTrigger * s_highSpeedMultiplier));

		const float yawRotation = -xAxisRight * s_yawRotSpeed * timeDeltaF;
		m_yaw += yawRotation;

		const float pitchRotation = yAxisRight * s_pitchRotSpeed * timeDeltaF;
		m_pitch += pitchRotation;

		// build direction from pitch, yaw
		glm::vec3 downZ(0.0f, 0.0f, -1.0f);
		m_lookDirection = glm::normalize(glm::rotateX(downZ, m_pitch));		
		m_lookDirection = glm::normalize(glm::rotateY(m_lookDirection, m_yaw));

		// build right + up vectors
		const glm::vec3 upY(0.0f, 1.0f, 0.0f);
		m_right = glm::cross(m_lookDirection, upY);

		// move forward
		const float forward = yAxisLeft * s_forwardSpeed  * moveSpeedMulti * timeDeltaF;
		m_position += m_lookDirection * forward;

		// strafe
		const float strafe = xAxisLeft * s_strafeSpeed * moveSpeedMulti * timeDeltaF;
		m_position += m_right * strafe;
	}
}