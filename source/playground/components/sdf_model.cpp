#include "sdf_model.h"
#include "core/log.h"

COMPONENT_SCRIPTS(SDFModel,
	"SetBounds", &SDFModel::SetBounds,
	"SetResolution", &SDFModel::SetResolution,
	"SetSampleFunction", &SDFModel::SetSampleFunction
)

// Used by surface nets and DC
// For the cell defined by (p0->p0+cellSize), detect sign changes across edges and calculate position where d=0, returns a list of points
void ExtractEdgeIntersections(glm::vec3 p, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], std::vector<glm::vec3>& outIntersections)
{
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			if ((corners[crnX][crnY][0].distance > 0.0f) != (corners[crnX][crnY][1].distance > 0.0f))
			{
				float zero = (0.0f - corners[crnX][crnY][0].distance) / (corners[crnX][crnY][1].distance - corners[crnX][crnY][0].distance);
				outIntersections.push_back({ p.x + crnX * cellSize.x, p.y + crnY * cellSize.y, p.z + zero * cellSize.z });
			}
		}
	}
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnZ = 0; crnZ < 2; ++crnZ)
		{
			if ((corners[crnX][0][crnZ].distance > 0.0f) != (corners[crnX][1][crnZ].distance > 0.0f))
			{
				float zero = (0.0f - corners[crnX][0][crnZ].distance) / (corners[crnX][1][crnZ].distance - corners[crnX][0][crnZ].distance);
				outIntersections.push_back({ p.x + crnX * cellSize.x, p.y + zero * cellSize.y, p.z + crnZ * cellSize.z });
			}
		}
	}
	for (int crnY = 0; crnY < 2; ++crnY)
	{
		for (int crnZ = 0; crnZ < 2; ++crnZ)
		{
			if ((corners[0][crnY][crnZ].distance > 0.0f) != (corners[1][crnY][crnZ].distance > 0.0f))
			{
				float zero = (0.0f - corners[0][crnY][crnZ].distance) / (corners[1][crnY][crnZ].distance - corners[0][crnY][crnZ].distance);
				outIntersections.push_back({ p.x + zero * cellSize.x, p.y + crnY * cellSize.y, p.z + crnZ * cellSize.z });
			}
		}
	}
}

bool SDFModel::FindVertex_Blocky(glm::vec3 p0, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3& outVertex) const
{
	// blocky is simply, if ANY edges have a sign change, generate a vertex in the center of the cell
	bool addVertex = false;
	for (int crnX = 0; crnX < 2 && !addVertex; ++crnX)
	{
		for (int crnY = 0; crnY < 2 && !addVertex; ++crnY)
		{
			if ((corners[crnX][crnY][0].distance > 0.0f) != (corners[crnX][crnY][1].distance > 0.0f))
			{
				addVertex = true;
			}
		}
	}
	for (int crnX = 0; crnX < 2 && !addVertex; ++crnX)
	{
		for (int crnZ = 0; crnZ < 2 && !addVertex; ++crnZ)
		{
			if ((corners[crnX][0][crnZ].distance > 0.0f) != (corners[crnX][1][crnZ].distance > 0.0f))
			{
				addVertex = true;
			}
		}
	}
	for (int crnY = 0; crnY < 2&& !addVertex; ++crnY)
	{
		for (int crnZ = 0; crnZ < 2&& !addVertex; ++crnZ)
		{
			if ((corners[0][crnY][crnZ].distance > 0.0f) != (corners[1][crnY][crnZ].distance > 0.0f))
			{
				addVertex = true;
			}
		}
	}
	outVertex = p0 + cellSize * 0.5f;
	return addVertex;
}

bool SDFModel::FindVertex_SurfaceNet(glm::vec3 p, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3& outVertex) const
{
	// detect sign changes on edges, extract vertices per edge at the zero point
	// return the average of all the found vertices
	std::vector<glm::vec3> foundPositions;	// yeah its slow, sue me
	ExtractEdgeIntersections(p, cellSize, corners, foundPositions);
	glm::vec3 averagePos(0.0f);
	for (const auto v : foundPositions)
	{
		averagePos += v;
	}
	const bool foundOne = foundPositions.size() > 0;
	if(foundOne)
	{
		averagePos = averagePos / (float)foundPositions.size();
	}
	outVertex = averagePos;
	return foundOne;
}

void SDFModel::SampleCorners(glm::vec3 p, glm::vec3 cellSize, SDFModel::Sample(&corners)[2][2][2]) const
{
	// p0 = bottom left point
	// c--------c
	// |        |
	// |        |
	// p0-------c
	assert(m_sampleFunction != nullptr);
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			for (int crnZ = 0; crnZ < 2; ++crnZ)
			{
				m_sampleFunction(p.x + crnX * cellSize.x, p.y + crnY * cellSize.y, p.z + crnZ * cellSize.z, corners[crnX][crnY][crnZ]);
			}
		}
	}
}

glm::vec3 SDFModel::SampleNormal(float x, float y, float z, float sampleDelta)
{
	glm::vec3 normal(0.0f, 1.0f, 0.0f);
	if (m_sampleFunction != nullptr)
	{
		Sample samples[6];
		m_sampleFunction(x + sampleDelta, y, z, samples[0]);
		m_sampleFunction(x - sampleDelta, y, z, samples[1]);
		m_sampleFunction(x, y + sampleDelta, z, samples[2]);
		m_sampleFunction(x, y - sampleDelta, z, samples[3]);
		m_sampleFunction(x, y, z + sampleDelta, samples[4]);
		m_sampleFunction(x, y, z - sampleDelta, samples[5]);
		normal.x = (samples[0].distance - samples[1].distance) / 2 / sampleDelta;
		normal.y = (samples[2].distance - samples[3].distance) / 2 / sampleDelta;
		normal.z = (samples[4].distance - samples[5].distance) / 2 / sampleDelta;
		normal = glm::normalize(normal);
	}
	return normal;
}

void SDFModel::SetSampleScriptFunction(sol::protected_function fn)
{
	auto wrappedFn = [fn](float x, float y, float z, Sample& s) {
		sol::protected_function_result result = fn(x,y,z,s);
		if (!result.valid())
		{
			SDE_LOG("Failed to call sample function");
		}
	};
	m_sampleFunction = std::move(wrappedFn);
}