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
	"SetBoundsMax", &SDFModel::SetBoundsMax,
	"Remesh", &SDFModel::Remesh
)

inline uint32_t CellToIndex(int x, int y, int z, glm::ivec3 res)
{
	return x + (y * res.x) + (z * res.x * res.y);
}

auto CellHash = [](int x, int y, int z) -> uint64_t
{
	const uint64_t maxBitsPerAxis = 20;
	const uint64_t mask = ((uint64_t)1 << maxBitsPerAxis) - 1;
	uint64_t key = ((uint64_t)x & mask) |
		(((uint64_t)y & mask) << maxBitsPerAxis) |
		((uint64_t)z & mask) << (maxBitsPerAxis * 2);
	return key;
};

void SDFModel::SampleGrid(std::vector<Sample>& allSamples)
{
	SDE_PROF_EVENT();
	auto cellSize = GetCellSize();
	Sample s;
	allSamples.resize(GetResolution().x * GetResolution().y * (uint64_t)GetResolution().z);
	for (int x = 0; x < GetResolution().x; ++x)
	{
		for (int y = 0; y < GetResolution().y; ++y)
		{
			for (int z = 0; z < GetResolution().z; ++z)
			{
				const auto index = CellToIndex(x, y, z, GetResolution());
				const glm::vec3 p = GetBoundsMin() + cellSize * glm::vec3(x, y, z);
				m_sampleFunction(p.x, p.y, p.z, s);	
				allSamples[index] = s;
			}
		}
	}
}

// collect vertices for each cell containing an edge transition
void SDFModel::FindVertices(const std::vector<Sample>& samples, SDFDebug& dbg, MeshMode mode,
	std::vector<glm::vec3>& outV, std::vector<glm::vec3>& outN, robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert)
{
	SDE_PROF_EVENT();
	auto cellSize = GetCellSize();
	SDFModel::Sample corners[2][2][2];	// evaluate the function at each corner of the cell
	bool drawCellNormals = dbg.ShouldDrawNormals();
	for (int x = 0; x < GetResolution().x - 1; ++x)
	{
		for (int y = 0; y < GetResolution().y - 1; ++y)
		{
			for (int z = 0; z < GetResolution().z - 1; ++z)
			{
				glm::vec3 p = GetBoundsMin() + cellSize * glm::vec3(x, y, z);
				bool addVertex = false;
				glm::vec3 cellVertex;	// this will be the output position
				SampleCorners(x, y, z, samples, corners);
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
					glm::vec3 normal = SampleNormal(cellVertex.x, cellVertex.y, cellVertex.z, glm::compMin(cellSize) * 0.5f);
					dbg.DrawCellVertex(cellVertex);
					dbg.DrawCellNormal(cellVertex, normal);
					outV.push_back(cellVertex);
					outN.push_back(normal);
					cellToVert[CellHash(x, y, z)] = outV.size() - 1;
				}
			}
		}
	}
}

void SDFModel::FindQuads(const std::vector<Sample>& samples, const std::vector<glm::vec3>& v, const std::vector<glm::vec3>& n,
	robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert, QuadFn fn)
{
	SDE_PROF_EVENT();
	// for each cell, generate quads from edges with sign differences
	// by joining the vertices in neighbour cells for a particular edge
	auto cellSize = GetCellSize();
	for (int x = 0; x < GetResolution().x - 1; ++x)
	{
		for (int y = 0; y < GetResolution().y - 1; ++y)
		{
			for (int z = 0; z < GetResolution().z - 1; ++z)
			{
				glm::vec3 p = GetBoundsMin() + cellSize * glm::vec3(x, y, z);
				if (x > 0 && y > 0)
				{
					SDFModel::Sample s0, s1;
					s0 = samples[CellToIndex(x, y, z, GetResolution())];
					s1 = samples[CellToIndex(x, y, z + 1, GetResolution())];
					if ((s0.distance > 0.0f) != (s1.distance > 0.0f))
					{
						auto i0 = cellToVert[CellHash(x - 1, y - 1, z)];
						auto i1 = cellToVert[CellHash(x - 0, y - 1, z)];
						auto i2 = cellToVert[CellHash(x - 0, y - 0, z)];
						auto i3 = cellToVert[CellHash(x - 1, y - 0, z)];
						auto v0 = v[i0];	auto v1 = v[i1];
						auto v2 = v[i2];	auto v3 = v[i3];
						auto n0 = n[i0];	auto n1 = n[i1];
						auto n2 = n[i2];	auto n3 = n[i3];
						if (s1.distance > 0.0f)
						{
							fn(v0, v1, v2, v3, n0, n1, n2, n3);
						}
						else
						{
							fn(v3, v2, v1, v0, n3, n2, n1, n0);
						}
					}
				}
				if (x > 0 && z > 0)
				{
					SDFModel::Sample s0, s1;
					s0 = samples[CellToIndex(x, y, z, GetResolution())];
					s1 = samples[CellToIndex(x, y + 1, z, GetResolution())];
					if ((s0.distance > 0.0f) != (s1.distance > 0.0f))
					{
						auto i0 = cellToVert[CellHash(x - 1, y, z - 1)];
						auto i1 = cellToVert[CellHash(x - 0, y, z - 1)];
						auto i2 = cellToVert[CellHash(x - 0, y, z - 0)];
						auto i3 = cellToVert[CellHash(x - 1, y, z - 0)];
						auto v0 = v[i0];	auto v1 = v[i1];
						auto v2 = v[i2];	auto v3 = v[i3];
						auto n0 = n[i0];	auto n1 = n[i1];
						auto n2 = n[i2];	auto n3 = n[i3];
						if (s0.distance > 0.0f)
						{
							fn(v0, v1, v2, v3, n0, n1, n2, n3);
						}
						else
						{
							fn(v3, v2, v1, v0, n3, n2, n1, n0);
						}
					}
				}
				if (y > 0 && z > 0)
				{
					SDFModel::Sample s0, s1;
					s0 = samples[CellToIndex(x, y, z, GetResolution())];
					s1 = samples[CellToIndex(x + 1, y, z, GetResolution())];
					if ((s0.distance > 0.0f) != (s1.distance > 0.0f))
					{
						auto i0 = cellToVert[CellHash(x, y - 1, z - 1)];
						auto i1 = cellToVert[CellHash(x, y - 0, z - 1)];
						auto i2 = cellToVert[CellHash(x, y - 0, z - 0)];
						auto i3 = cellToVert[CellHash(x, y - 1, z - 0)];
						auto v0 = v[i0];	auto v1 = v[i1];
						auto v2 = v[i2];	auto v3 = v[i3];
						auto n0 = n[i0];	auto n1 = n[i1];
						auto n2 = n[i2];	auto n3 = n[i3];
						if (s1.distance > 0.0f)
						{
							fn(v0, v1, v2, v3, n0, n1, n2, n3);
						}
						else
						{
							fn(v3, v2, v1, v0, n3, n2, n1, n0);
						}
					}
				}
			}
		}
	}
}

void SDFModel::UpdateMesh(SDFDebug& dbg)
{
	SDE_PROF_EVENT();
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

	// sample the density function at all points on the fixed grid
	// without this we need to call it WAY too many times!
	std::vector<SDFModel::Sample> cachedSamples;
	SampleGrid(cachedSamples);

	std::vector<glm::vec3> vertices;	// 1 vertex per cell
	std::vector<glm::vec3> normals;		// much faster to calculate them once per vertex
	robin_hood::unordered_map<uint64_t, uint32_t> cellToVertex;	// map of cell to vertex (uses cell hash below)
	FindVertices(cachedSamples, dbg, m_meshMode, vertices, normals, cellToVertex);

	Render::MeshBuilder builder;
	builder.AddVertexStream(3);	// pos
	builder.AddVertexStream(3);	// normal
	auto quad = [&builder,&dbg](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
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
		dbg.DrawQuad(v0, v1, v2, v3);
	};
	builder.BeginChunk();
	// Generate quads from a combination of the samples and indexed normals and positions
	FindQuads(cachedSamples, vertices, normals, cellToVertex, quad);
	builder.EndChunk();
	{
		SDE_PROF_EVENT("CreateMesh");
		builder.CreateMesh(*m_mesh);
		builder.CreateVertexArray(*m_mesh);
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

// Used by surface nets and DC
// For the cell defined by (p0->p0+cellSize), detect sign changes across edges and calculate position where d=0, returns a list of points (max 1 per edge)
void ExtractEdgeIntersections(glm::vec3 p, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3* outIntersections, int& outCount)
{
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			if ((corners[crnX][crnY][0].distance > 0.0f) != (corners[crnX][crnY][1].distance > 0.0f))
			{
				float zero = (0.0f - corners[crnX][crnY][0].distance) / (corners[crnX][crnY][1].distance - corners[crnX][crnY][0].distance);
				outIntersections[outCount++] = { p.x + crnX * cellSize.x, p.y + crnY * cellSize.y, p.z + zero * cellSize.z };
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
				outIntersections[outCount++] = { p.x + crnX * cellSize.x, p.y + zero * cellSize.y, p.z + crnZ * cellSize.z };
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
				outIntersections[outCount++] = { p.x + zero * cellSize.x, p.y + crnY * cellSize.y, p.z + crnZ * cellSize.z };
			}
		}
	}
}

bool SDFModel::FindVertex_SurfaceNet(glm::vec3 p, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3& outVertex) const
{
	// detect sign changes on edges, extract vertices per edge at the zero point
	// return the average of all the found vertices
	glm::vec3 foundPositions[16];
	int intersections = 0;
	ExtractEdgeIntersections(p, cellSize, corners, foundPositions, intersections);
	glm::vec3 averagePos(0.0f);
	for (int i=0;i<intersections;++i)
	{
		averagePos += foundPositions[i];
	}
	const bool foundOne = intersections > 0;
	if(foundOne)
	{
		averagePos = averagePos / (float)intersections;
	}
	outVertex = averagePos;
	return foundOne;
}

void SDFModel::SampleCorners(int x, int y, int z, const std::vector<Sample>& v, SDFModel::Sample(&corners)[2][2][2]) const
{
	for (int crnX = 0; crnX < 2; ++crnX)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			for (int crnZ = 0; crnZ < 2; ++crnZ)
			{
				auto index = CellToIndex(x + crnX, y + crnY, z + crnZ, GetResolution());
				corners[crnX][crnY][crnZ] = v[index];
			}
		}
	}
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