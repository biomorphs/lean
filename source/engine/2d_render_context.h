#pragma once

#include "core/glm_headers.h"
#include "texture_manager.h"
#include "shader_manager.h"
#include "render/mesh.h"

namespace Engine
{
	class RenderContext2D
	{
	public:
		void Reset(glm::vec2 dimensions);
		void DrawTriangle(const glm::vec2 pos[3], int zIndex, const glm::vec2 uv[3], const glm::vec4 colour[3], TextureHandle t);
		void DrawQuad(glm::vec2 origin, int zIndex, glm::vec2 dimensions, glm::vec2 uv0, glm::vec2 uv1, glm::vec4 colour, TextureHandle texture);
		void Render(Render::Device& d);
		glm::vec2 GetDimensions() const { return m_viewportDimensions; }

	private:
		struct Triangle
		{
			glm::vec2 m_position[3];
			glm::vec2 m_uv[3];
			glm::vec4 m_colour[3];
			TextureHandle m_texture;
			int m_zIndex;
		};

		void Initialise(glm::vec2 dimensions);
		void Shutdown();

		glm::vec2 m_viewportDimensions = { 0,0 };
		std::vector<Triangle> m_thisFrameTriangles;

		ShaderHandle m_shader;
		std::vector<std::unique_ptr<Render::RenderBuffer>> m_triangleData;
		std::vector<std::unique_ptr<Render::RenderBuffer>> m_vertices;
		std::vector<std::unique_ptr<Render::VertexArray>> m_vertexArrays;
		int m_currentBuffer = 0;
		int m_maxVertices = 1024 * 16;
		int m_maxBuffers = 3;
	};

}