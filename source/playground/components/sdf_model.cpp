#include "sdf_model.h"
#include "render/mesh.h"
#include "render/mesh_builder.h"
#include "core/log.h"

COMPONENT_SCRIPTS(SDFModel,
	"SetBounds", &SDFModel::SetBounds,
	"SetResolution", &SDFModel::SetResolution,
	"SetSampleFunction", &SDFModel::SetSampleFunction,
	"SetShader", &SDFModel::SetShader,
	"SetBoundsMin", &SDFModel::SetBoundsMin,
	"SetBoundsMax", &SDFModel::SetBoundsMax
)

void SDFModel::UpdateMesh(MeshMode mode, SDFDebug& dbg)
{
	if (m_mesh == nullptr || m_remesh)
	{
		m_mesh = std::make_unique<Render::Mesh>();
		m_remesh = false;
	}
	else
	{
		return;
	}
	const auto cellSize = (GetBoundsMax() - GetBoundsMin()) / glm::vec3(GetResolution());
	std::vector<glm::vec3> vertices;	// 1 vertex per cell
	robin_hood::unordered_map<uint64_t, uint32_t> cellToVertex;	// map of cell to vertex (uses cell hash below)
	auto cellHash = [](int x, int y, int z) -> uint64_t
	{
		const uint64_t maxBitsPerAxis = 20;
		const uint64_t mask = ((uint64_t)1 << maxBitsPerAxis) - 1;
		uint64_t key = ((uint64_t)x & mask) |
			(((uint64_t)y & mask) << maxBitsPerAxis) |
			((uint64_t)z & mask) << (maxBitsPerAxis * 2);
		return key;
	};

	// collect vertices for each cell containing an edge transition
	for (int x = 0; x <= GetResolution().x - 1; ++x)
	{
		for (int y = 0; y <= GetResolution().y - 1; ++y)
		{
			for (int z = 0; z <= GetResolution().z - 1; ++z)
			{
				glm::vec3 p = GetBoundsMin() + cellSize * glm::vec3(x, y, z);
				bool addVertex = false;
				glm::vec3 cellVertex;	// this will be the output position
				SDFModel::Sample corners[2][2][2];	// evaluate the function at each corner of the cell
				SampleCorners(p, cellSize, corners);
				dbg.DrawCorners(p, corners);
				switch (mode)
				{
				case Blocky:
					addVertex = FindVertex_Blocky(p, cellSize, corners, cellVertex);
					break;
				case SurfaceNet:
					addVertex = FindVertex_SurfaceNet(p, cellSize, corners, cellVertex);
					break;
				default:
					assert(false);
				};

				if (addVertex)
				{
					dbg.DrawCellVertex(cellVertex);
					glm::vec3 normal = SampleNormal(cellVertex.x, cellVertex.y, cellVertex.z, glm::compMin(cellSize) * 0.5f);
					dbg.DrawCellNormal(cellVertex, normal);
					vertices.push_back(cellVertex);
					cellToVertex[cellHash(x, y, z)] = vertices.size() - 1;
				}
			}
		}
	}

	Render::MeshBuilder builder;
	builder.AddVertexStream(3);	// pos
	builder.AddVertexStream(3);	// normal
	auto quad = [&builder](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
		glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3)
	{
		builder.BeginTriangle();
		builder.SetStreamData(0, v0, v1, v2);
		builder.SetStreamData(1, n0, n1, n2);
		builder.EndTriangle();
		builder.BeginTriangle();
		builder.SetStreamData(0, v2, v3, v0);
		builder.SetStreamData(1, n2, n3, n0);
		builder.EndTriangle();
	};
	builder.BeginChunk();

	// now for each cell, generate quads from edges with sign differences
	// by joining the vertices in neighbour cells for a particular edge
	float normalSampleDelta = glm::compMin(cellSize * 0.5f);
	for (int x = 0; x <= GetResolution().x - 1; ++x)
	{
		for (int y = 0; y <= GetResolution().y - 1; ++y)
		{
			for (int z = 0; z <= GetResolution().z - 1; ++z)
			{
				glm::vec3 p = GetBoundsMin() + cellSize * glm::vec3(x, y, z);
				if (x > 0 && y > 0)
				{
					SDFModel::Sample s0, s1;
					m_sampleFunction(p.x, p.y, p.z, s0);
					m_sampleFunction(p.x, p.y, p.z + cellSize.z, s1);
					if ((s0.distance > 0.0f) != (s1.distance > 0.0f))
					{
						auto v0 = vertices[cellToVertex[cellHash(x - 1, y - 1, z)]];
						auto v1 = vertices[cellToVertex[cellHash(x - 0, y - 1, z)]];
						auto v2 = vertices[cellToVertex[cellHash(x - 0, y - 0, z)]];
						auto v3 = vertices[cellToVertex[cellHash(x - 1, y - 0, z)]];
						auto n0 = SampleNormal(v0.x, v0.y, v0.z, normalSampleDelta);
						auto n1 = SampleNormal(v1.x, v1.y, v1.z, normalSampleDelta);
						auto n2 = SampleNormal(v2.x, v2.y, v2.z, normalSampleDelta);
						auto n3 = SampleNormal(v3.x, v3.y, v3.z, normalSampleDelta);
						dbg.DrawQuad(v0, v1, v2, v3);
						if (s1.distance > 0.0f)
						{
							quad(v0, v1, v2, v3, n0, n1, n2, n3);
						}
						else
						{
							quad(v3, v2, v1, v0, n3, n2, n1, n0);
						}
					}
				}
				if (x > 0 && z > 0)
				{
					SDFModel::Sample s0, s1;
					m_sampleFunction(p.x, p.y, p.z, s0);
					m_sampleFunction(p.x, p.y + cellSize.y, p.z, s1);
					if ((s0.distance > 0.0f) != (s1.distance > 0.0f))
					{
						auto v0 = vertices[cellToVertex[cellHash(x - 1, y, z - 1)]];
						auto v1 = vertices[cellToVertex[cellHash(x - 0, y, z - 1)]];
						auto v2 = vertices[cellToVertex[cellHash(x - 0, y, z - 0)]];
						auto v3 = vertices[cellToVertex[cellHash(x - 1, y, z - 0)]];
						auto n0 = SampleNormal(v0.x, v0.y, v0.z, normalSampleDelta);
						auto n1 = SampleNormal(v1.x, v1.y, v1.z, normalSampleDelta);
						auto n2 = SampleNormal(v2.x, v2.y, v2.z, normalSampleDelta);
						auto n3 = SampleNormal(v3.x, v3.y, v3.z, normalSampleDelta);
						dbg.DrawQuad(v0, v1, v2, v3);
						if (s0.distance > 0.0f)
						{
							quad(v0, v1, v2, v3, n0, n1, n2, n3);
						}
						else
						{
							quad(v3, v2, v1, v0, n3, n2, n1, n0);
						}
					}
				}
				if (y > 0 && z > 0)
				{
					SDFModel::Sample s0, s1;
					m_sampleFunction(p.x, p.y, p.z, s0);
					m_sampleFunction(p.x + cellSize.x, p.y, p.z, s1);
					if ((s0.distance > 0.0f) != (s1.distance > 0.0f))
					{
						auto v0 = vertices[cellToVertex[cellHash(x, y - 1, z - 1)]];
						auto v1 = vertices[cellToVertex[cellHash(x, y - 0, z - 1)]];
						auto v2 = vertices[cellToVertex[cellHash(x, y - 0, z - 0)]];
						auto v3 = vertices[cellToVertex[cellHash(x, y - 1, z - 0)]];
						auto n0 = SampleNormal(v0.x, v0.y, v0.z, normalSampleDelta);
						auto n1 = SampleNormal(v1.x, v1.y, v1.z, normalSampleDelta);
						auto n2 = SampleNormal(v2.x, v2.y, v2.z, normalSampleDelta);
						auto n3 = SampleNormal(v3.x, v3.y, v3.z, normalSampleDelta);
						dbg.DrawQuad(v0, v1, v2, v3);
						if (s1.distance > 0.0f)
						{
							quad(v0, v1, v2, v3, n0, n1, n2, n3);
						}
						else
						{
							quad(v3, v2, v1, v0, n3, n2, n1, n0);
						}
					}
				}
			}
		}
	}

	builder.EndChunk();
	builder.CreateMesh(*m_mesh);
	builder.CreateVertexArray(*m_mesh);
}

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