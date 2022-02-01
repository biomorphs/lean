#pragma once

#include "core/glm_headers.h"

namespace Engine
{
	// adapted from https://tavianator.com/2015/ray_box_nan.html
	bool RayIntersectsAABB(glm::vec3 rayStart, glm::vec3 rayEnd, glm::vec3 bmin, glm::vec3 bmax, float& t);
}