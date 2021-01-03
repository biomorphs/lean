#pragma once

#include "core/glm_headers.h"
#include "shader_manager.h"
#include <memory>
#include <functional>

namespace Render
{
	class Mesh;
}

namespace Engine
{
	class Renderer;
	class DebugRender
	{
	public:
		DebugRender(ShaderManager* sm);
		~DebugRender() = default;

		void AddLines(const glm::vec4* v, const glm::vec4* c, uint32_t count);
		void AddAxisAtPoint(const glm::vec4& point, float scale = 1.0f);
		void AddBox(glm::vec3 center, glm::vec3 dimensions, glm::vec4 colour);

		void PushToRenderer(Renderer&);
	private:
		bool CreateMesh();
		void PushLinesToMesh(Render::Mesh& target);
		void AddLinesInternal(const __m128* posBuffer, const __m128* colBuffer, uint32_t count);

		static const uint32_t c_maxLines = 1024 * 1024 * 1;
		uint32_t m_currentLines;
		std::unique_ptr<glm::vec4, std::function<void(glm::vec4*)>> m_posBuffer;
		std::unique_ptr<glm::vec4, std::function<void(glm::vec4*)>> m_colBuffer;
		ShaderHandle m_shader;
		static const uint32_t c_meshBuffers = 2;	// triple-buffered
		std::unique_ptr<Render::Mesh> m_renderMesh[c_meshBuffers];		
		uint32_t m_currentWriteMesh;
	};
}