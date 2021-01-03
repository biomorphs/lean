#pragma once
#include "core/glm_headers.h"

namespace Engine
{
	class Light
	{
	public:
		glm::vec4 m_colour;			// w = ambient strength
		glm::vec4 m_position;		// w = 0, light is directional (looking at 0,0) : w=1, point light
		glm::vec3 m_attenuation;	// constant, linear, quadratic
	};
}