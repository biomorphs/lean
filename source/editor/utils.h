#pragma once

#include "core/glm_headers.h"

namespace EditorUtils
{
	void MouseCursorToWorldspaceRay(float rayDistance, glm::vec3& rayStart, glm::vec3& rayEnd);
}