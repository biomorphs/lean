#include "input_system.h"
#include "core/profiler.h"
#include "engine/system_manager.h"
#include "engine/event_system.h"
#include "engine/script_system.h"
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>
#include <SDL_mouse.h>
#include <SDL_events.h>
#include <algorithm>
#include <map>
#include <assert.h>

namespace Engine
{
	const std::map<SDL_Keycode, Key> c_keyMapping = {
		{ SDLK_0, KEY_0 },
		{ SDLK_1, KEY_1 },
		{ SDLK_2, KEY_2 },
		{ SDLK_3, KEY_3 },
		{ SDLK_4, KEY_4 },
		{ SDLK_5, KEY_5 },
		{ SDLK_6, KEY_6 },
		{ SDLK_7, KEY_7 },
		{ SDLK_8, KEY_8 },
		{ SDLK_9, KEY_9 },
		{ SDLK_a, KEY_a },
		{ SDLK_b, KEY_b },
		{ SDLK_c, KEY_c },
		{ SDLK_d, KEY_d },
		{ SDLK_e, KEY_e },
		{ SDLK_f, KEY_f },
		{ SDLK_g, KEY_g },
		{ SDLK_h, KEY_h },
		{ SDLK_i, KEY_i },
		{ SDLK_j, KEY_j },
		{ SDLK_k, KEY_k },
		{ SDLK_l, KEY_l },
		{ SDLK_m, KEY_m },
		{ SDLK_n, KEY_n },
		{ SDLK_o, KEY_o },
		{ SDLK_p, KEY_p },
		{ SDLK_q, KEY_q },
		{ SDLK_r, KEY_r },
		{ SDLK_s, KEY_s },
		{ SDLK_t, KEY_t },
		{ SDLK_u, KEY_u },
		{ SDLK_v, KEY_v },
		{ SDLK_w, KEY_w },
		{ SDLK_x, KEY_x },
		{ SDLK_y, KEY_y },
		{ SDLK_z, KEY_z },
		{ SDLK_LSHIFT, KEY_LSHIFT },
		{ SDLK_RSHIFT, KEY_RSHIFT },
		{ SDLK_SPACE, KEY_SPACE },
		{ SDLK_LEFT, KEY_LEFT },
		{ SDLK_RIGHT, KEY_RIGHT },
		{ SDLK_UP, KEY_UP },
		{ SDLK_DOWN, KEY_DOWN },
		{ SDLK_LCTRL, KEY_LCTRL },
		{ SDLK_RCTRL, KEY_RCTRL },
		{ SDLK_ESCAPE, KEY_ESCAPE }
	};

	const std::map<std::string, Key> c_keyStringMapping = {
		{"KEY_0", Key::KEY_0},
		{"KEY_1", Key::KEY_1},
		{"KEY_2", Key::KEY_2},
		{"KEY_3", Key::KEY_3},
		{"KEY_4", Key::KEY_4},
		{"KEY_5", Key::KEY_5},
		{"KEY_6", Key::KEY_6},
		{"KEY_7", Key::KEY_7},
		{"KEY_8", Key::KEY_8},
		{"KEY_9", Key::KEY_9},
		{"KEY_a", Key::KEY_a},
		{"KEY_b", Key::KEY_b},
		{"KEY_c", Key::KEY_c},
		{"KEY_d", Key::KEY_d},
		{"KEY_e", Key::KEY_e},
		{"KEY_f", Key::KEY_f},
		{"KEY_g", Key::KEY_g},
		{"KEY_h", Key::KEY_h},
		{"KEY_i", Key::KEY_i},
		{"KEY_j", Key::KEY_j},
		{"KEY_k", Key::KEY_k},
		{"KEY_l", Key::KEY_l},
		{"KEY_m", Key::KEY_m},
		{"KEY_n", Key::KEY_n},
		{"KEY_o", Key::KEY_o},
		{"KEY_p", Key::KEY_p},
		{"KEY_q", Key::KEY_q},
		{"KEY_r", Key::KEY_r},
		{"KEY_s", Key::KEY_s},
		{"KEY_t", Key::KEY_t},
		{"KEY_u", Key::KEY_u},
		{"KEY_v", Key::KEY_v},
		{"KEY_w", Key::KEY_w},
		{"KEY_x", Key::KEY_x},
		{"KEY_y", Key::KEY_y},
		{"KEY_z", Key::KEY_z},
		{"KEY_LSHIFT", Key::KEY_LSHIFT },
		{"KEY_RSHIFT", Key::KEY_RSHIFT },
		{"KEY_SPACE", Key::KEY_SPACE },
		{"KEY_LEFT", Key::KEY_LEFT },
		{"KEY_RIGHT", Key::KEY_RIGHT },
		{"KEY_UP", Key::KEY_UP },
		{"KEY_DOWN", Key::KEY_DOWN },
		{"KEY_LCTRL", Key::KEY_LCTRL },
		{"KEY_RCTRL", Key::KEY_RCTRL },
		{"KEY_ESCAPE", Key::KEY_ESCAPE}
	};

	InputSystem::InputSystem()
		: m_controllerAxisDeadZone( 0.5f )
	{
	}

	InputSystem::~InputSystem()
	{
	}

	void InputSystem::OnSystemEvent(void* e)
	{
		auto theEvent = (SDL_Event*)e;
		if (theEvent->type == SDL_MOUSEWHEEL)
		{
			m_currentMouseScroll = theEvent->wheel.y;
		}
		if (theEvent->type == SDL_KEYDOWN || theEvent->type == SDL_KEYUP)
		{
			auto mappedKey = c_keyMapping.find(theEvent->key.keysym.sym);
			if (mappedKey != c_keyMapping.end())
			{
				m_keysState.m_keyPressed[mappedKey->second] = theEvent->key.state == SDL_PRESSED ? true : false;
			}
		}
	}

	bool InputSystem::PreInit()
	{
		auto eventSystem = Engine::GetSystem<EventSystem>("Events");
		eventSystem->RegisterEventHandler([this](void* e) { OnSystemEvent(e); });

		m_scripts = Engine::GetSystem<ScriptSystem>("Script");

		return true;
	}

	bool InputSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();
		m_mouseState.m_wheelScroll = m_currentMouseScroll;
		m_currentMouseScroll = 0;
		EnumerateControllers();
		UpdateControllerState();
		UpdateMouseState();
		return true;
	}

	const ControllerRawState InputSystem::ControllerState(uint32_t padIndex) const
	{
		if (padIndex < m_controllers.size())
		{
			return m_controllers[padIndex].m_padState;
		}
		else
		{
			return {};
		}
	}

	void InputSystem::UpdateMouseState()
	{
		SDE_PROF_EVENT();

		m_mouseState.m_buttonState = 0;
		uint32_t mouseButtons = SDL_GetMouseState(&m_mouseState.m_cursorX, &m_mouseState.m_cursorY);
		if (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT))
		{
			m_mouseState.m_buttonState |= MouseButtons::LeftButton;
		}
		if (mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		{
			m_mouseState.m_buttonState |= MouseButtons::MiddleButton;
		}
		if (mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT))
		{
			m_mouseState.m_buttonState |= MouseButtons::RightButton;
		}
	}

	void InputSystem::UpdateControllerState()
	{
		SDE_PROF_EVENT();

		for (auto& padDesc : m_controllers)
		{
			SDL_GameController* controller = (SDL_GameController*)padDesc.m_sdlController;

			// helper macros for button / axis stuff
			#define GET_SDL_BUTTON_STATE(sdlBtn, sdeBtn)	\
				SDL_GameControllerGetButton(controller, sdlBtn) ? sdeBtn : 0
			#define GET_SDL_AXIS_STATE(sdlAxis)				\
				((float)SDL_GameControllerGetAxis(controller, sdlAxis) / 32768.0f)

			uint32_t buttonState = 0;
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_A, ControllerButtons::A);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_B, ControllerButtons::B);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_X, ControllerButtons::X);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_Y, ControllerButtons::Y);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_BACK, ControllerButtons::Back);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_GUIDE, ControllerButtons::Guide);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_START, ControllerButtons::Start);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_LEFTSTICK, ControllerButtons::LeftStick);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_RIGHTSTICK, ControllerButtons::RightStick);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_LEFTSHOULDER, ControllerButtons::LeftShoulder);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, ControllerButtons::RightShoulder);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_DPAD_UP, ControllerButtons::DPadUp);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_DPAD_DOWN, ControllerButtons::DPadDown);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_DPAD_LEFT, ControllerButtons::DPadLeft);
			buttonState |= GET_SDL_BUTTON_STATE( SDL_CONTROLLER_BUTTON_DPAD_RIGHT, ControllerButtons::DPadRight);
			padDesc.m_padState.m_buttonState = buttonState;

			// Triggers are 0 - 32k, sticks are -32k to 32k, normalised
			padDesc.m_padState.m_leftTrigger = GET_SDL_AXIS_STATE(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
			padDesc.m_padState.m_rightTrigger = GET_SDL_AXIS_STATE(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
			padDesc.m_padState.m_leftStickAxes[0] = ApplyDeadZone(GET_SDL_AXIS_STATE(SDL_CONTROLLER_AXIS_LEFTX), m_controllerAxisDeadZone);
			padDesc.m_padState.m_leftStickAxes[1] = -ApplyDeadZone(GET_SDL_AXIS_STATE(SDL_CONTROLLER_AXIS_LEFTY), m_controllerAxisDeadZone);
			padDesc.m_padState.m_rightStickAxes[0] = ApplyDeadZone(GET_SDL_AXIS_STATE(SDL_CONTROLLER_AXIS_RIGHTX), m_controllerAxisDeadZone);
			padDesc.m_padState.m_rightStickAxes[1] = -ApplyDeadZone(GET_SDL_AXIS_STATE(SDL_CONTROLLER_AXIS_RIGHTY), m_controllerAxisDeadZone);
		}
	}

	inline float InputSystem::ApplyDeadZone(float input, float deadZone) const
	{
		if (input < deadZone && input > -deadZone)
		{
			return 0.0f;
		}
		else
		{
			return input;
		}
	}

	bool InputSystem::IsKeyDown(const char* keyStr)
	{
		const auto foundKeyLookup = c_keyStringMapping.find(keyStr);
		assert(foundKeyLookup != c_keyStringMapping.end());
		if (foundKeyLookup != c_keyStringMapping.end())
		{
			return GetKeyboardState().m_keyPressed[foundKeyLookup->second];
		}
		else
		{
			return false;
		}
	}

	bool InputSystem::Initialise()
	{
		SDE_PROF_EVENT();
		EnumerateControllers();

		auto scripts = m_scripts->Globals()["Input"].get_or_create<sol::table>();
		scripts["IsKeyPressed"] = [this](const char* keyStr) {
			return m_keysEnabled && IsKeyDown(keyStr);
		};

		return true;
	}

	void InputSystem::EnumerateControllers()
	{
		SDE_PROF_EVENT();
		m_controllers.clear();
		for (int i = 0; i < SDL_NumJoysticks(); ++i) 
		{
			if (SDL_IsGameController(i)) 
			{
				SDL_GameController *controller = SDL_GameControllerOpen(i);
				if (controller) 
				{
					ControllerDesc newController;
					newController.m_sdlController = controller;
					memset(&newController.m_padState, 0, sizeof(newController.m_padState));
					m_controllers.push_back(newController);
				}
			}
		}
	}
}