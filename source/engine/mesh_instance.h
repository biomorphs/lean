#pragma once
#include <stdint.h>
#include "core/glm_headers.h"
#include <intrin.h>

namespace Render
{
	class ShaderProgram;
	class Mesh;
	class Material;
	class VertexArray;
	class RenderBuffer;
	class MeshChunk;
}

namespace Engine
{
	class Mesh;
	class ShaderProgram;

	template<class UserInstanceData>
	class MeshInstance
	{
	public:
		MeshInstance() = default;
		MeshInstance(const MeshInstance&) = default;
		MeshInstance(MeshInstance&&) = default;
		MeshInstance& operator=(const MeshInstance& other) = default;
		MeshInstance& operator=(MeshInstance&& other) = default;

		__m128i m_sortKey;
		glm::mat4 m_transform;
		glm::vec3 m_aabbMin;
		glm::vec3 m_aabbMax;
		Render::ShaderProgram* m_shader;
		const Render::VertexArray* m_va;
		const Render::RenderBuffer* m_ib;
		const Render::MeshChunk* m_chunks;
		uint32_t m_chunkCount;
		const Render::Material* m_meshMaterial;
		const Render::Material* m_instanceMaterial;

		UserInstanceData m_pid;	// custom per-instance data
	};
}