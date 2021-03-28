#pragma once

namespace Engine
{
	namespace SDFRaycaster
	{

		bool Raycast(glm::vec3 p0, glm::vec3 p1, float step, SDFMeshBuilder::SampleFn fn, float& tOut, int& matHit)
	}
}