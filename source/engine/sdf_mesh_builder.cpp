#include "sdf_mesh_builder.h"
#include "core/profiler.h"
#include "render/mesh_builder.h"

#define QEF_INCLUDE_IMPL
#include <qef_simd.h>

// grid based mesh extraction
// inspired by https://www.boristhebrave.com/2018/04/15/dual-contouring-tutorial/
// Stage 1: Vertex generation
//	for each point on a arbitrary grid
//		sample the SDF at each corner of a 'cell' containing that point
//		for each edge running between those corner points
//			if the density function sign changes (+/-), generate a vertex for that cell
//				Minecraft-style: use the cell center point
//				Surface nets: average of all intersection points
//				Dual contouring: solve a set of linear equations describing lines running from each point perpendicular to the normal. clamp the vertex to the cell bounds
//				add the vertex to a list, and also keep a map of cell [x,y,z] -> vertex for later
// Stage 2: Edge building
// for each point on the grid
//	for each edge in the cell (only 3 edges per point running along major axes)
//		if this edge has a sign change between corners
//			find the vertices from adjacent cells sharing this edge (4 of them) and make a quad
// build mesh data from quads

namespace Engine
{
	inline uint32_t CellToIndex(int x, int y, int z, glm::ivec3 res)
	{
		return x + (y * res.x) + (z * res.x * res.y);
	}

	inline uint64_t CellHash(int x, int y, int z)
	{
		const uint64_t maxBitsPerAxis = 20;
		const uint64_t mask = ((uint64_t)1 << maxBitsPerAxis) - 1;
		uint64_t key = ((uint64_t)x & mask) |
			(((uint64_t)y & mask) << maxBitsPerAxis) |
			((uint64_t)z & mask) << (maxBitsPerAxis * 2);
		return key;
	}

	std::unique_ptr<Render::MeshBuilder> SDFMeshBuilder::MakeMeshBuilder(MeshMode mode, SampleFn fn, glm::vec3 origin, glm::vec3 cellSize, glm::ivec3 sampleResolution, bool smoothNormals, Debug& debug)
	{
		m_fn = fn;
		m_origin = origin;
		m_cellSize = cellSize;
		m_resolution = sampleResolution;
		m_mode = mode;
		m_debug = &debug;

		// sample the density function at all points on the fixed grid
		std::vector<Sample> cachedSamples;
		SampleGrid(cachedSamples);

		// Find vertices 1 per cell 
		std::vector<glm::vec3> vertices;	// 1 vertex per cell
		std::vector<glm::vec3> normals;		// much faster to calculate them once per vertex
		robin_hood::unordered_map<uint64_t, uint32_t> cellToVertex;	// map of cell to vertex (uses cell hash below)
		FindVertices(cachedSamples, vertices, normals, cellToVertex);

		// Build the mesh(builder)
		auto builder = std::make_unique<Render::MeshBuilder>();
		builder->AddVertexStream(3);	// pos
		builder->AddVertexStream(3);	// normal
		auto quad = [&builder, smoothNormals, this](glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
			glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3)
		{
			if (!smoothNormals)
			{
				auto l0 = v1 - v0;
				auto l1 = v2 - v0;
				auto normal = glm::normalize(glm::cross(l0, l1));
				n0 = normal;
				n1 = normal;
				n2 = normal;
				n3 = normal;
			}
			builder->BeginTriangle();
			builder->SetStreamData(0, v0, v1, v2);
			builder->SetStreamData(1, n0, n1, n2);
			builder->EndTriangle();
			builder->BeginTriangle();
			builder->SetStreamData(0, v2, v3, v0);
			builder->SetStreamData(1, n2, n3, n0);
			builder->EndTriangle();
			m_debug->OnQuad(v0, v1, v2, v3, n0, n1, n2, n3);
		};
		builder->BeginChunk();
		FindQuads(cachedSamples, vertices, normals, cellToVertex, quad);
		builder->EndChunk();

		return builder;
	}

	void SDFMeshBuilder::FindQuads(const std::vector<Sample>& samples, const std::vector<glm::vec3>& v, const std::vector<glm::vec3>& n,
		robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert, QuadFn fn)
	{
		SDE_PROF_EVENT();
		// for each cell, generate quads from edges with sign differences
		// by joining the vertices in neighbour cells for a particular edge
		for (int z = 0; z < m_resolution.z - 1; ++z)
		{
			for (int y = 0; y < m_resolution.y - 1; ++y)
			{
				for (int x = 0; x < m_resolution.x - 1; ++x)
				{
					glm::vec3 p = m_origin + m_cellSize * glm::vec3(x, y, z);
					if (x > 0 && y > 0)
					{
						Sample s0, s1;
						s0 = samples[CellToIndex(x, y, z, m_resolution)];
						s1 = samples[CellToIndex(x, y, z + 1, m_resolution)];
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
						Sample s0, s1;
						s0 = samples[CellToIndex(x, y, z, m_resolution)];
						s1 = samples[CellToIndex(x, y + 1, z, m_resolution)];
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
						Sample s0, s1;
						s0 = samples[CellToIndex(x, y, z, m_resolution)];
						s1 = samples[CellToIndex(x + 1, y, z, m_resolution)];
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

	void SDFMeshBuilder::SampleCorners(int x, int y, int z, const std::vector<Sample>& v, Sample(&corners)[2][2][2]) const
	{
		for (int crnZ = 0; crnZ < 2; ++crnZ)
		{
			for (int crnY = 0; crnY < 2; ++crnY)
			{
				for (int crnX = 0; crnX < 2; ++crnX)
				{
					auto index = CellToIndex(x + crnX, y + crnY, z + crnZ, m_resolution);
					corners[crnX][crnY][crnZ] = v[index];
				}
			}
		}
	}

	void SDFMeshBuilder::SampleGrid(std::vector<Sample>& allSamples)
	{
		SDE_PROF_EVENT();
		Sample s;
		allSamples.resize((uint64_t)m_resolution.x * (uint64_t)m_resolution.y * (uint64_t)m_resolution.z);

		for (int z = 0; z < m_resolution.z; ++z)
		{
			for (int y = 0; y < m_resolution.y; ++y)
			{
				for (int x = 0; x < m_resolution.x; ++x)
				{
					const auto index = CellToIndex(x, y, z, m_resolution);
					assert(index < allSamples.size());
					const glm::vec3 p = m_origin + m_cellSize * glm::vec3(x, y, z);
					std::tie(allSamples[index].distance, allSamples[index].material) = m_fn(p.x, p.y, p.z);
				}
			}
		}
	}

	// collect vertices for each cell containing an edge transition
	void SDFMeshBuilder::FindVertices(const std::vector<Sample>& samples, 
		std::vector<glm::vec3>& outV, std::vector<glm::vec3>& outN, robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert)
	{
		SDE_PROF_EVENT();
		Sample corners[2][2][2];	// evaluate the function at each corner of the cell
		for (int z = 0; z < m_resolution.z - 1; ++z)
		{
			for (int y = 0; y < m_resolution.y - 1; ++y)
			{
				for (int x = 0; x < m_resolution.x - 1; ++x)
				{
					glm::vec3 p = m_origin + m_cellSize * glm::vec3(x, y, z);
					bool addVertex = false;
					SampleCorners(x, y, z, samples, corners);

					glm::vec3 cellVertex;	// this will be the output position
					switch (m_mode)
					{
					case Blocky:
						addVertex = FindVertex_Blocky(p, m_cellSize, corners, cellVertex);
						break;
					case SurfaceNet:
						addVertex = FindVertex_SurfaceNet(p, m_cellSize, corners, cellVertex);
						break;
					case DualContour:
						addVertex = FindVertex_DualContour(p, m_cellSize, corners, cellVertex);
						break;
					default:
						assert(false);
					};
					if (addVertex)
					{
						auto v = glm::compMax(m_cellSize) * m_normalSmoothness;
						glm::vec3 normal = SampleNormal(cellVertex.x, cellVertex.y, cellVertex.z, v);
						m_debug->OnCellVertex(cellVertex, normal);
						outV.push_back(cellVertex);
						outN.push_back(normal);
						cellToVert[CellHash(x, y, z)] = outV.size() - 1;
					}
				}
			}
		}
	}

	// Used by surface nets and DC
	// For the cell defined by (p0->p0+cellSize), detect sign changes across edges and calculate position where d=0, returns a list of points (max 1 per edge)
	void SDFMeshBuilder::ExtractEdgeIntersections(glm::vec3 p, const Sample(&corners)[2][2][2], glm::vec3* outIntersections, int& outCount) const
	{
		for (int crnX = 0; crnX < 2; ++crnX)
		{
			for (int crnY = 0; crnY < 2; ++crnY)
			{
				if ((corners[crnX][crnY][0].distance > 0.0f) != (corners[crnX][crnY][1].distance > 0.0f))
				{
					float zero = (0.0f - corners[crnX][crnY][0].distance) / (corners[crnX][crnY][1].distance - corners[crnX][crnY][0].distance);
					outIntersections[outCount++] = { p.x + crnX * m_cellSize.x, p.y + crnY * m_cellSize.y, p.z + zero * m_cellSize.z };
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
					outIntersections[outCount++] = { p.x + crnX * m_cellSize.x, p.y + zero * m_cellSize.y, p.z + crnZ * m_cellSize.z };
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
					outIntersections[outCount++] = { p.x + zero * m_cellSize.x, p.y + crnY * m_cellSize.y, p.z + crnZ * m_cellSize.z };
				}
			}
		}
	}

	bool SDFMeshBuilder::FindVertex_DualContour(glm::vec3 p, glm::vec3 cellSize, const Sample(&corners)[2][2][2], glm::vec3& outVertex) const
	{
		// detect sign changes on edges, extract vertices per edge at the zero point
		glm::vec3 foundPositions[16];
		int intersections = 0;
		ExtractEdgeIntersections(p, corners, foundPositions, intersections);

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
			if (glm::any(glm::lessThan(outVertex, p - errorTolerance)) || glm::any(glm::greaterThan(outVertex, p + errorTolerance + cellSize)))
			{
				if (s_doClamp)
				{
					outVertex = glm::clamp(outVertex, p, p + cellSize);
				}
				else if (s_useSurfaceNetFallback)
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
		return intersections > 0;
	}

	bool SDFMeshBuilder::FindVertex_SurfaceNet(glm::vec3 p, glm::vec3 cellSize, const Sample(&corners)[2][2][2], glm::vec3& outVertex) const
	{
		// detect sign changes on edges, extract vertices per edge at the zero point
		// return the average of all the found vertices
		glm::vec3 foundPositions[16];
		int intersections = 0;
		ExtractEdgeIntersections(p, corners, foundPositions, intersections);
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

	bool SDFMeshBuilder::FindVertex_Blocky(glm::vec3 p0, glm::vec3 cellSize, const Sample(&corners)[2][2][2], glm::vec3& outVertex) const
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
		for (int crnY = 0; crnY < 2 && !addVertex; ++crnY)
		{
			for (int crnZ = 0; crnZ < 2 && !addVertex; ++crnZ)
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

	glm::vec3 SDFMeshBuilder::SampleNormal(float x, float y, float z, float sampleDelta) const
	{
		glm::vec3 normal(0.0f, 1.0f, 0.0f);
		if (m_fn != nullptr)
		{
			Sample samples[6];
			std::tie(samples[0].distance, samples[0].material) = m_fn(x + sampleDelta, y, z);
			std::tie(samples[1].distance, samples[1].material) = m_fn(x - sampleDelta, y, z);
			std::tie(samples[2].distance, samples[2].material) = m_fn(x, y + sampleDelta, z);
			std::tie(samples[3].distance, samples[3].material) = m_fn(x, y - sampleDelta, z);
			std::tie(samples[4].distance, samples[4].material) = m_fn(x, y, z + sampleDelta);
			std::tie(samples[5].distance, samples[5].material) = m_fn(x, y, z - sampleDelta);
			normal.x = (samples[0].distance - samples[1].distance) / 2 / sampleDelta;
			normal.y = (samples[2].distance - samples[3].distance) / 2 / sampleDelta;
			normal.z = (samples[4].distance - samples[5].distance) / 2 / sampleDelta;
			normal = glm::normalize(normal);
		}
		return normal;
	}
}
