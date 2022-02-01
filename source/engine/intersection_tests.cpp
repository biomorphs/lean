#include "intersection_tests.h"

namespace Engine
{
	bool RayIntersectsAABB(glm::vec3 rayStart, glm::vec3 rayEnd, glm::vec3 bmin, glm::vec3 bmax, float& t)
	{
		const auto dir = glm::normalize(rayEnd - rayStart);
		double tmin = -FLT_MAX;
		double tmax = FLT_MAX;
		for (int i = 0; i < 3; ++i)
		{
			if (dir[i] != 0.0f)
			{
				const double invDir = 1.0f / dir[i];
				const double t1 = (bmin[i] - rayStart[i]) * invDir;
				const double t2 = (bmax[i] - rayStart[i]) * invDir;
				tmin = glm::max(tmin, glm::min(t1, t2));
				tmax = glm::min(tmax, glm::max(t1, t2));
			}
			else if (rayStart[i] <= bmin[i] || rayStart[i] >= bmax[i])
			{
				return false;
			}
		}

		const float dist = glm::length(rayEnd - rayStart);
		t = tmin / dist;
		return tmax > glm::max(0.0,tmin);
	}
}