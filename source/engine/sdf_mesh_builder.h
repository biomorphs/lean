#pragma once
#include "core/glm_headers.h"
#include "sdf.h"
#include <robin_hood.h>
#include <vector>
#include <functional>

namespace Render
{
	class MeshBuilder;
}

namespace Engine
{
	class SDFMeshBuilder
	{
	public:
		enum MeshMode
		{
			Blocky,
			SurfaceNet,
			DualContour
		};
		struct Debug
		{
			virtual void OnQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3) {}
			virtual void OnCellVertex(glm::vec3 p, glm::vec3 n) {}
		};
		std::unique_ptr<Render::MeshBuilder> MakeMeshBuilder(MeshMode mode,SDF::SampleFn fn, glm::vec3 origin, glm::vec3 cellSize, glm::ivec3 sampleResolution, float smoothNormals = 1.0f, Debug& debug = Debug());
	private:
		glm::ivec3 m_resolution;
		glm::vec3 m_origin;
		glm::vec3 m_cellSize;
		bool m_generateAO = false;	// very slow
		float m_normalSmoothness = 1.0f;
		MeshMode m_mode = Blocky;
		Debug* m_debug = nullptr;
		SDF::SampleFn m_fn;
		std::vector<glm::vec3> m_spherePoints;

		struct Sample {
			float distance = -1.0f;
			uint8_t material = (uint8_t)-1;
		};
		void GenerateAO(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, std::vector<float>& ao);
		glm::vec3 SampleNormal(float x, float y, float z, float sampleDelta = 0.01f) const;
		void SampleGrid(std::vector<Sample>& allSamples);
		void SampleCorners(int x, int y, int z, const std::vector<Sample>& v, Sample(&corners)[2][2][2]) const;
		void FindVertices(const std::vector<Sample>& samples, std::vector<glm::vec3>& outV, std::vector<glm::vec3>& outN, robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert);
		bool FindVertex_Blocky(glm::vec3 p0, glm::vec3 cellSize, const Sample(&corners)[2][2][2], glm::vec3& outVertex) const;
		bool FindVertex_SurfaceNet(glm::vec3 p0, glm::vec3 cellSize, const Sample(&corners)[2][2][2], glm::vec3& outVertex) const;
		bool FindVertex_DualContour(glm::vec3 p0, glm::vec3 cellSize, const Sample(&corners)[2][2][2], glm::vec3& outVertex) const;
		void ExtractEdgeIntersections(glm::vec3 p, const Sample(&corners)[2][2][2], glm::vec3* outIntersections, int& outCount) const;
		using QuadFn = std::function<void(int,int,int,int)>;
		void FindQuads(const std::vector<Sample>& samples, robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert, QuadFn fn);
	};
}