#pragma once

#include "core/glm_headers.h"

namespace Engine
{
	// adapted from https://tavianator.com/2015/ray_box_nan.html
	bool RayIntersectsAABB(glm::vec3 rayStart, glm::vec3 rayEnd, glm::vec3 bmin, glm::vec3 bmax, float& t);

	// find closest points between 2 lines (returned as distance along each ray). returns false if no valid result (paralel lines)
	bool GetNearPointsBetweenLines(glm::vec3 p0start, glm::vec3 p0end, glm::vec3 p1start, glm::vec3 p1end, glm::vec3& p0, glm::vec3& p1);
}