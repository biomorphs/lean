#pragma once

#include "core/glm_headers.h"
#include "mesh.h"
#include <stdint.h>

namespace Render
{
	// Helper for creating mesh objects
	class MeshBuilder
	{
	public:
		MeshBuilder() = default;
		~MeshBuilder() = default;

		bool HasData();

		// Step 1: Define streams
		uint32_t AddVertexStream(int32_t componentCount, size_t reserveMemory = 0);

		// Step 2: Define chunks
		void BeginChunk();
		
		// Step 3: Add triangles to chunks
		void BeginTriangle();

		// Step 4: Add data for each triangle/stream
		void SetStreamData(uint32_t vertexStream, float v0, float v1, float v2);
		void SetStreamData(uint32_t vertexStream, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2);
		void SetStreamData(uint32_t vertexStream, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
		void SetStreamData(uint32_t vertexStream, const glm::vec4& v0, const glm::vec4& v1, const glm::vec4& v2);

		void EndTriangle();
		
		void EndChunk();

		// Step 5: Mesh creation
		bool CreateMesh(Mesh& target, bool createDynamicMesh=true, size_t minVbSize = 0);
		bool CreateVertexArray(Mesh& mesh);	// this must happen on the main thread!

	private:
		bool ShouldRecreateMesh(Mesh& target, size_t minVbSize);
		void RecreateMesh(Mesh& target, bool createDynamicMesh, size_t minVbSize);

		struct StreamDesc
		{
			int32_t m_componentCount = 0;
			std::vector<float> m_streamData;
		};
		struct ChunkDesc
		{
			uint32_t m_firstVertex;
			uint32_t m_lastVertex;
		};

		ChunkDesc m_currentChunk = { 0,0 };
		int m_currentVertexIndex = 0;
		std::vector<StreamDesc> m_streams;
		std::vector<ChunkDesc> m_chunks;
	};
}