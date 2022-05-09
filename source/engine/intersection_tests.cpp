#include "intersection_tests.h"

namespace Engine
{
	bool GetNearPointsBetweenLines(glm::vec3 p0start, glm::vec3 p0end, glm::vec3 p1start, glm::vec3 p1end, glm::vec3& p0, glm::vec3& p1)
	{
		// adapted from https://en.wikipedia.org/wiki/Skew_lines#Nearest_points
		glm::vec3 dir0 = glm::normalize(p0start - p0end);
		glm::vec3 dir1 = glm::normalize(p1start - p1end);

		// The cross product of dir0 and dir1 is perpendicular to both rays
		glm::vec3 perp = glm::cross(dir0, dir1);

		// the plane formed by the translations of Line 2 along 'perp' contains the point p1start
		// and is perpendicular to n2 = dir1 cross perp
		glm::vec3 n1 = glm::cross(dir0, perp);
		glm::vec3 n2 = glm::cross(dir1, perp);
		// Therefore, the intersecting point of Line 1 with the above - mentioned plane, 
		// which is also the point on Line 1 that is nearest to Line 2 is given by...

		p0 = p0start + ((glm::dot(p1start - p0start, n2) / glm::dot(dir0, n2)) * dir0);
		p1 = p1start + ((glm::dot(p0start - p1start, n1) / glm::dot(dir1, n1)) * dir1);

		return true;
	}

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