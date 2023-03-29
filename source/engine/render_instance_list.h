#pragma once

#include "core/glm_headers.h"
#include "model.h"
#include <vector>
#include <functional>
#include <atomic>

namespace Render
{
	class UniformBuffer;
	class VertexArray;
	class MeshChunk;
	class ShaderProgram;
	class RenderBuffer;
	class Material;
	class Mesh;
}

namespace Engine
{
	struct ShaderHandle;
	struct ModelHandle;

	struct PerInstanceData
	{
		glm::vec4 m_diffuseOpacity;
		glm::vec4 m_specular;
		glm::vec4 m_shininess;
		uint64_t m_diffuseTexture;		// resident handles, be careful
		uint64_t m_normalsTexture;
		uint64_t m_specularTexture;
	};

	class RenderInstanceList
	{
	public:
		struct TransformBounds {
			glm::mat4 m_transform;
			glm::vec3 m_aabbMin;
			glm::vec3 m_aabbMax;
		};
		struct DrawData {
			Render::ShaderProgram* m_shader;
			const Render::VertexArray* m_va;
			const Render::RenderBuffer* m_ib;
			const Render::Material* m_meshMaterial;
			const Render::MeshChunk* m_chunks;
			uint32_t m_chunkCount;
		};
		struct Entry {
			__m128i m_sortKey;
			uint32_t m_dataIndex;
		};
		std::vector<TransformBounds> m_transformBounds;
		std::vector<DrawData> m_drawData;
		std::vector<PerInstanceData> m_perInstanceData;
		std::vector<Entry> m_entries;
		std::atomic<uint32_t> m_count = 0;

		void Reserve(size_t count);				// resizes the arrays to this size, doesn't touch the index
		void Reset();
		uint32_t AddInstances(int instanceCount);	// returns start index into arrays, or -1 on fail
		void SetInstance(uint32_t index, __m128i sortKey, const glm::mat4& trns, const Render::VertexArray* va, const Render::RenderBuffer* ib, const Render::MeshChunk* chunks,
			uint32_t chunkCount, const Render::Material* meshMaterial, Render::ShaderProgram* shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
			const PerInstanceData& pid);

		int m_maxInstances = 0;
	};

	class RenderInstances
	{
	public:
		void SubmitInstances(const __m128* positions, int count, const ModelHandle& model, const ShaderHandle& shader);
		void SubmitInstance(const glm::mat4& trns, const Render::Mesh& mesh, const ShaderHandle& shader, const Render::Material* matOverride = nullptr, glm::vec3 boundsMin = glm::vec3(-FLT_MAX), glm::vec3 boundsMax = glm::vec3(FLT_MAX));
		void SubmitInstance(const glm::mat4& trns, const ModelHandle& model, const ShaderHandle& shader, const Model::MeshPart::DrawData* partOverride=nullptr, uint32_t overrideCount = 0);

		void Reset();
		void Reserve(size_t count);

		RenderInstanceList m_opaquesDeferred;
		RenderInstanceList m_opaquesForward;
		RenderInstanceList m_transparents;
		RenderInstanceList m_shadowCasters;
	};
}