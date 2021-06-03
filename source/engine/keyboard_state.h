#pragma once

#include <array>
#include <string>

namespace Engine
{
	enum Key {
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		KEY_a,
		KEY_b,
		KEY_c,
		KEY_d,
		KEY_e,
		KEY_f,
		KEY_g,
		KEY_h,
		KEY_i,
		KEY_j,
		KEY_k,
		KEY_l,
		KEY_m,
		KEY_n,
		KEY_o,
		KEY_p,
		KEY_q,
		KEY_r,
		KEY_s,
		KEY_t,
		KEY_u,
		KEY_v,
		KEY_w,
		KEY_x,
		KEY_y,
		KEY_z,
		KEY_LSHIFT,
		KEY_RSHIFT,
		KEY_SPACE,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		KEY_MAX
	};
	
	struct KeyboardState
	{
		std::array<bool, KEY_MAX>  m_keyPressed = { false };
	};
}