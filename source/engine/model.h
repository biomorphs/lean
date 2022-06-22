#pragma once
#include "core/glm_headers.h"
#include "texture_manager.h"
#include "render/device.h"
#include "render/mesh.h"
#include <memory>

namespace Render
{
	class Mesh;
	class RenderBuffer;
	class VertexArray;
}

namespace Engine
{
	class Model
	{
	public:
		Model() = default;
		~Model() = default;

		glm::vec3& BoundsMin() { return m_boundsMin; }
		glm::vec3& BoundsMax() { return m_boundsMax; }
		const glm::vec3& BoundsMin() const { return m_boundsMin; }
		const glm::vec3& BoundsMax() const { return m_boundsMax; }

		struct MeshPart
		{
			glm::mat4 m_transform;
			glm::vec3 m_boundsMin;
			glm::vec3 m_boundsMax;
			Render::Material m_material;				// 'push' constants, use these sparingly if you draw a lot!
			struct DrawData {
				glm::vec4 m_diffuseOpacity = { 1.0f,1.0f,1.0f,1.0f };
				glm::vec4 m_specular = { 1.0f, 1.0f, 1.0f, 1.0f };	//r,g,b,strength
				glm::vec4 m_shininess;
				TextureHandle m_diffuseTexture;
				TextureHandle m_normalsTexture;
				TextureHandle m_specularTexture;
			};
			DrawData m_drawData;						// fast per-instance data
			std::vector<Render::MeshChunk> m_chunks;	// draw calls indexing into global buffers
		};
		const std::vector<MeshPart>& MeshParts() const { return m_meshParts; }
		std::vector<MeshPart>& MeshParts() { return m_meshParts; }

	private:
		std::vector<MeshPart> m_meshParts;
		glm::vec3 m_boundsMin = glm::vec3(FLT_MAX);
		glm::vec3 m_boundsMax = glm::vec3(-FLT_MAX);
	};
}