/*
SDLEngine
Matt Hoyle
*/

#include "mesh_builder.h"
#include "utils.h"
#include "core/glm_headers.h"
#include "core/profiler.h"
#include <cassert>

namespace Render
{
	MeshBuilder::MeshBuilder()
		: m_currentVertexIndex(0)
	{
	}

	MeshBuilder::~MeshBuilder()
	{
	}

	bool MeshBuilder::HasData()
	{
		if (m_chunks.size() > 0)
		{
			return (m_chunks[0].m_lastVertex != 0);
		}
		else
		{
			return 0;
		}
	}

	void MeshBuilder::SetStreamData(uint32_t vertexStream, float v0, float v1, float v2)
	{
		assert(vertexStream < m_streams.size());
		assert(m_streams[vertexStream].m_componentCount == 1);
		auto& streamData = m_streams[vertexStream].m_streamData;
		streamData.push_back(v0);
		streamData.push_back(v1);
		streamData.push_back(v2);
	}

	void MeshBuilder::SetStreamData(uint32_t vertexStream, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2)
	{
		assert(vertexStream < m_streams.size());
		assert(m_streams[vertexStream].m_componentCount == 2);

		auto& streamData = m_streams[vertexStream].m_streamData;
		streamData.insert(streamData.end(), glm::value_ptr(v0), glm::value_ptr(v0) + 2);
		streamData.insert(streamData.end(), glm::value_ptr(v1), glm::value_ptr(v1) + 2);
		streamData.insert(streamData.end(), glm::value_ptr(v2), glm::value_ptr(v2) + 2);
	}

	void MeshBuilder::SetStreamData(uint32_t vertexStream, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
	{
		assert(vertexStream < m_streams.size());
		assert(m_streams[vertexStream].m_componentCount == 3);

		auto& streamData = m_streams[vertexStream].m_streamData;
		streamData.insert(streamData.end(), glm::value_ptr(v0), glm::value_ptr(v0) + 3);
		streamData.insert(streamData.end(), glm::value_ptr(v1), glm::value_ptr(v1) + 3);
		streamData.insert(streamData.end(), glm::value_ptr(v2), glm::value_ptr(v2) + 3);
	}

	void MeshBuilder::SetStreamData(uint32_t vertexStream, const glm::vec4& v0, const glm::vec4& v1, const glm::vec4& v2)
	{
		assert(vertexStream < m_streams.size());
		assert(m_streams[vertexStream].m_componentCount == 4);

		auto& streamData = m_streams[vertexStream].m_streamData;
		streamData.insert(streamData.end(), glm::value_ptr(v0), glm::value_ptr(v0) + 4);
		streamData.insert(streamData.end(), glm::value_ptr(v1), glm::value_ptr(v1) + 4);
		streamData.insert(streamData.end(), glm::value_ptr(v2), glm::value_ptr(v2) + 4);
	}

	uint32_t MeshBuilder::AddVertexStream(int32_t componentCount, size_t reserveMemory)
	{
		assert(componentCount <= 4);
		assert(m_chunks.size() == 0);
		
		StreamDesc newStream;
		newStream.m_componentCount = componentCount;
		newStream.m_streamData.reserve(reserveMemory);
		m_streams.push_back(newStream);

		return static_cast<uint32_t>( m_streams.size() - 1 );
	}

	void MeshBuilder::BeginTriangle()
	{
	}

	void MeshBuilder::EndTriangle()
	{
		m_currentVertexIndex += 3;

		// Make sure all streams have data
#ifdef SDE_DEBUG
		for (const auto& stream : m_streams)
		{
			assert((stream.m_streamData.size() / stream.m_componentCount) == m_currentVertexIndex);
		}
#endif
	}

	void MeshBuilder::BeginChunk()
	{
		assert(m_streams.size() > 0);

		m_currentChunk.m_firstVertex = m_currentVertexIndex;
		m_currentChunk.m_lastVertex = m_currentVertexIndex;
	}

	void MeshBuilder::EndChunk()
	{
		m_currentChunk.m_lastVertex = m_currentVertexIndex;
		if ((m_currentChunk.m_lastVertex - m_currentChunk.m_firstVertex) > 0)
		{
			m_chunks.push_back(std::move(m_currentChunk));
		}		
	}

	bool MeshBuilder::ShouldRecreateMesh(Mesh& target, size_t minVbSize)
	{
		const size_t c_maximumWaste = minVbSize;
		// If the mesh streams match, and the stream buffers are big enough, then we don't need to do anything
		auto& streams = target.GetStreams();

		if (streams.size() != m_streams.size())
		{
			return true;
		}

		for (int32_t streamIndex = 0; streamIndex < streams.size(); ++streamIndex)
		{
			auto thisStreamSize = m_streams[streamIndex].m_streamData.size() * sizeof(float);

			if (streams[streamIndex].GetSize() < thisStreamSize)
			{
				return true;
			}
			if (streams[streamIndex].GetSize() > (thisStreamSize + c_maximumWaste))
			{
				return true;
			}
		}

		return false;
	}

	void MeshBuilder::RecreateMesh(Mesh& target, bool createDynamicMesh, size_t minVbSize)
	{
		SDE_PROF_EVENT();

		auto& streams = target.GetStreams();
		for (auto& s : streams)
		{
			s.Destroy();
		}
		streams.clear();
		streams.resize(m_streams.size());
		int32_t streamIndex = 0;
		for (const auto& streamIt : m_streams)
		{
			auto& theBuffer = streams[streamIndex];
			auto streamSize = streamIt.m_streamData.size() * sizeof(float);
			if (streamSize < minVbSize)
			{
				streamSize = minVbSize;
			}

			// round up stream size to next minVbSize
			if (minVbSize != 0)
			{
				streamSize += (streamSize % minVbSize);
			}

			auto streamBuffer = (void*)streamIt.m_streamData.data();
			if (createDynamicMesh)
			{
				theBuffer.Create(streamBuffer,streamSize, RenderBufferType::VertexData, RenderBufferModification::Dynamic);
			}
			else
			{
				theBuffer.Create(streamBuffer,streamSize, RenderBufferType::VertexData, RenderBufferModification::Static);
			}
			++streamIndex;
		}

		streamIndex = 0;
	}

	bool MeshBuilder::CreateMesh(Mesh& target, bool createDynamicMesh, size_t minVbSize)
	{
		SDE_PROF_EVENT();

		auto& streams = target.GetStreams();
		auto& chunks = target.GetChunks();

		if (ShouldRecreateMesh(target, minVbSize))
		{
			RecreateMesh(target, createDynamicMesh, minVbSize);
		}
		else
		{
			int32_t streamIndex = 0;
			for (const auto& streamIt : m_streams)
			{
				auto& theBuffer = streams[streamIndex];
				auto streamSize = streamIt.m_streamData.size() * sizeof(float);
				theBuffer.SetData(0, streamSize, (void*)streamIt.m_streamData.data());
				++streamIndex;
			}
		}

		// Populate chunks. We always rebuild this data
		chunks.clear();
		chunks.reserve(m_chunks.size());
		for (const auto& chunk : m_chunks)
		{
			chunks.emplace_back(chunk.m_firstVertex, chunk.m_lastVertex - chunk.m_firstVertex, Render::PrimitiveType::Triangles);
		}

		return true;
	}

	bool MeshBuilder::CreateVertexArray(Mesh& mesh)
	{
		auto& va = mesh.GetVertexArray();
		va.Destroy();
		int32_t streamIndex = 0;
		auto& streams = mesh.GetStreams();
		for (const auto& streamIt : m_streams)
		{
			va.AddBuffer(streamIndex, &streams[streamIndex], VertexDataType::Float, streamIt.m_componentCount);
			++streamIndex;
		}
		return va.Create();
	}
}