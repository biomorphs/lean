#include "sdf_model.h"
#include "render/mesh.h"
#include "render/mesh_builder.h"
#include "core/log.h"

#define QEF_INCLUDE_IMPL
#include <qef_simd.h>

COMPONENT_SCRIPTS(SDFModel,
	"SetBounds", &SDFModel::SetBounds,
	"SetResolution", &SDFModel::SetResolution,
	"SetSampleFunction", &SDFModel::SetSampleScriptFunction,
	"SetShader", &SDFModel::SetShader,
	"SetBoundsMin", &SDFModel::SetBoundsMin,
	"SetBoundsMax", &SDFModel::SetBoundsMax,
	"Remesh", &SDFModel::Remesh,
	"SetDebugEnabled", &SDFModel::SetDebugEnabled,
	"GetDebugEnabled", &SDFModel::GetDebugEnabled,
	"SetMeshBlocky", &SDFModel::SetMeshBlocky,
	"SetMeshSurfaceNet", &SDFModel::SetMeshSurfaceNet,
	"SetMeshDualContour", &SDFModel::SetMeshDualContour
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

	for (int z = 0; z < GetResolution().z; ++z)
	{
		for (int y = 0; y < GetResolution().y; ++y)
		{
			for (int x = 0; x < GetResolution().x; ++x)
			{
				const auto index = CellToIndex(x, y, z, GetResolution());
				assert(index < allSamples.size());
				const glm::vec3 p = GetBoundsMin() + cellSize * glm::vec3(x, y, z);
				std::tie(allSamples[index].distance, allSamples[index].material) = m_sampleFunction(p.x, p.y, p.z);
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
	const auto res = GetResolution();
	const auto minBounds = GetBoundsMin();
	for (int z = 0; z < res.z-1; ++z)
	{
		for (int y = 0; y < res.y-1; ++y)
		{
			for (int x = 0; x < res.x-1; ++x)
			{
				glm::vec3 p = minBounds + cellSize * glm::vec3(x, y, z);
				bool addVertex = false;
				SampleCorners(x, y, z, samples, corners);
				dbg.DrawCellCorner(p, corners[0][0][0].distance);
				if (x + 1 >= res.x - 1)
				{
					dbg.DrawCellCorner(minBounds + cellSize * glm::vec3(x + 1, y, z), corners[1][0][0].distance);
				}
				if (y + 1 >= res.y - 1)
				{
					dbg.DrawCellCorner(minBounds + cellSize * glm::vec3(x, y + 1, z), corners[0][1][0].distance);
				}
				if (z + 1 >= res.z - 1)
				{
					dbg.DrawCellCorner(minBounds + cellSize * glm::vec3(x, y, z + 1), corners[0][0][1].distance);
				}

				glm::vec3 cellVertex;	// this will be the output position
				switch (mode)
				{
				case Blocky:
					addVertex = FindVertex_Blocky(p, cellSize, corners, cellVertex);
					break;
				case SurfaceNet:
					addVertex = FindVertex_SurfaceNet(p, cellSize, corners, cellVertex);
					break;
				case DualContour:
					addVertex = FindVertex_DualContour(p, cellSize, corners, cellVertex);
					break;
				default:
					assert(false);
				};
				if (addVertex)
				{
					auto v = glm::compMax(cellSize) * m_normalSmoothness;
					glm::vec3 normal = SampleNormal(cellVertex.x, cellVertex.y, cellVertex.z, v);
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
	for (int z = 0; z < GetResolution().z - 1; ++z)
	{
		for (int y = 0; y < GetResolution().y - 1; ++y)
		{
			for (int x = 0; x < GetResolution().x - 1; ++x)
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

static int s_dcSuccess = 0;
static int s_dcFailure = 0;

void SDFModel::UpdateMesh(SDFDebug& dbg)
{
	s_dcSuccess = 0;
	s_dcFailure = 0;

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

	// Find vertices 1 per cell 
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
		dbg.DrawQuad(v0, v1, v2, v3, n0, n1, n2, n3);
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

	if (m_meshMode == DualContour)
	{
		//SDE_LOG("Success: %d, Failure: %d", s_dcSuccess, s_dcFailure);
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

bool SDFModel::FindVertex_DualContour(glm::vec3 p, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3& outVertex) const
{
	// detect sign changes on edges, extract vertices per edge at the zero point
	glm::vec3 foundPositions[16];
	int intersections = 0;
	ExtractEdgeIntersections(p, cellSize, corners, foundPositions, intersections);

	// Get normals for each of the sample points + push data into registers
	__m128 foundPositions128[16];
	__m128 foundNormals128[16];
	auto v = glm::compMin(cellSize) * 0.1f;		// we need to sample normals at high frequency if possible
	for (int i = 0; i < intersections; ++i)
	{
		auto foundNormal = SampleNormal(foundPositions[i].x, foundPositions[i].y, foundPositions[i].z, v);
		foundPositions128[i] = _mm_set_ps(1.0f, foundPositions[i].z, foundPositions[i].y, foundPositions[i].x);
		foundNormals128[i] = _mm_set_ps(0.0f, foundNormal.z, foundNormal.y, foundNormal.x);
	}

	// Now find a vertex by solving the QEF of the points and normals
	if (intersections > 0)
	{
		__m128 solvedPosition;
		float qefError = qef_solve_from_points(foundPositions128, foundNormals128, intersections, &solvedPosition);
		outVertex.x = solvedPosition.m128_f32[0];
		outVertex.y = solvedPosition.m128_f32[1];
		outVertex.z = solvedPosition.m128_f32[2];

		// if the vertex is out of cells bounds, we have a few options
		// clamp to cell bounds (we lose sharp features, may fix crazy meshes)
		// fallback to surface net
		// schmidtz particle aproach(?)
		static bool s_doClamp = false;
		static bool s_useSurfaceNetFallback = true;
		const glm::vec3 errorTolerance = cellSize * 0.25f;
		if (glm::any(glm::lessThan(outVertex, p-errorTolerance)) || glm::any(glm::greaterThan(outVertex, p + errorTolerance + cellSize)))
		{	
			++s_dcFailure;
			if (s_doClamp)
			{
				outVertex = glm::clamp(outVertex, p, p + cellSize);
			}
			else if(s_useSurfaceNetFallback)
			{
				glm::vec3 averagePos(0.0f);
				for (int i = 0; i < intersections; ++i)
				{
					averagePos += foundPositions[i];
				}
				const bool foundOne = intersections > 0;
				if (foundOne)
				{
					averagePos = averagePos / (float)intersections;
				}
				outVertex = averagePos;
				return foundOne;
			}
		}
	}
	++s_dcSuccess;
	return intersections > 0;
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
	const auto res = GetResolution();
	for (int crnZ = 0; crnZ < 2; ++crnZ)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			for (int crnX = 0; crnX < 2; ++crnX)
			{
				auto index = CellToIndex(x + crnX, y + crnY, z + crnZ, res);
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
	for (int crnZ = 0; crnZ < 2; ++crnZ)
	{
		for (int crnY = 0; crnY < 2; ++crnY)
		{
			for (int crnX = 0; crnX < 2; ++crnX)
			{
				auto pos = glm::vec3(p.x + crnX * cellSize.x, p.y + crnY * cellSize.y, p.z + crnZ * cellSize.z);
				std::tie(corners[crnX][crnY][crnZ].distance, corners[crnX][crnY][crnZ].material) = m_sampleFunction(pos.x, pos.y, pos.z);
			}
		}
	}
}

glm::vec3 SDFModel::SampleNormal(float x, float y, float z, float sampleDelta) const
{
	glm::vec3 normal(0.0f, 1.0f, 0.0f);
	if (m_sampleFunction != nullptr)
	{
		Sample samples[6];
		std::tie(samples[0].distance, samples[0].material) = m_sampleFunction(x + sampleDelta, y, z);
		std::tie(samples[1].distance, samples[1].material) = m_sampleFunction(x - sampleDelta, y, z);
		std::tie(samples[2].distance, samples[2].material) = m_sampleFunction(x, y + sampleDelta, z);
		std::tie(samples[3].distance, samples[3].material) = m_sampleFunction(x, y - sampleDelta, z);
		std::tie(samples[4].distance, samples[4].material) = m_sampleFunction(x, y, z + sampleDelta);
		std::tie(samples[5].distance, samples[5].material) = m_sampleFunction(x, y, z - sampleDelta);
		normal.x = (samples[0].distance - samples[1].distance) / 2 / sampleDelta;
		normal.y = (samples[2].distance - samples[3].distance) / 2 / sampleDelta;
		normal.z = (samples[4].distance - samples[5].distance) / 2 / sampleDelta;
		normal = glm::normalize(normal);
	}
	return normal;
}

void SDFModel::SetSampleScriptFunction(sol::protected_function fn)
{
	auto wrappedFn = [fn](float x, float y, float z) -> std::tuple<float, int> {
		sol::protected_function_result result = fn(x,y,z);
		if (!result.valid())
		{
			SDE_LOG("Failed to call sample function");
		}
		std::tuple<float, int> r = result;
		return r;
	};
	m_sampleFunction = std::move(wrappedFn);
}