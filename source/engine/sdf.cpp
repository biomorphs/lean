#pragma once
#include "sdf.h"
#include "core/profiler.h"

namespace Engine
{
	namespace SDF
	{
		bool Raycast(glm::vec3 p0, glm::vec3 p1, float maxstep, SDF::SampleFn fn, float& tOut, int& matHit)
		{
			SDE_PROF_EVENT();
			assert(glm::length(p1 - p0) > 0.0f);
			assert(maxstep > 0.0f);

			int mat = 0;
			float p0distance = 0.0f;
			std::tie(p0distance, mat) = fn(p0.x, p0.y, p0.z);
			glm::vec3 dir = normalize(p1 - p0);
			float d = p0distance;
			glm::vec3 v = p0;
			while (true)
			{
				// we trust the SDF enough to step by the distance if we are in air
				// using a small step can still brute force if data is not trustworthy
				float step = glm::clamp(d, 0.05f, maxstep);
				v = v + dir * step;

				float t = glm::length(v - p0) / glm::length(p1 - p0);
				if (t > 1.0f)
				{
					return false;
				}

				std::tie(d, mat) = fn(v.x, v.y, v.z);
				if ((d > 0.0f) != (p0distance > 0.0f))
				{
					tOut = t;
					matHit = mat;
					return true;
				}
			}
		}
	}
}