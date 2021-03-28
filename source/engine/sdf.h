#pragma once
#include "core/glm_headers.h"
#include <functional>

namespace Engine
{
	namespace SDF
	{
		using SampleFn = std::function<std::tuple<float, int>(float, float, float)>;	// pos(3), out distance, out material

		// returns true on hit
		// note: detects change from solid/air OR air/solid
		// its up to you to detect if the initial point is solid or not!
		bool Raycast(glm::vec3 p0, glm::vec3 p1, float maxstep, SDF::SampleFn fn, float& tOut, int& matHit);
	}
}