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
		DebugRender();
		~DebugRender() = default;

		void DrawLine(glm::vec3 p0, glm::vec3 p1, glm::vec4 c);
		void AddLines(const glm::vec4* v, const glm::vec4* c, uint32_t count);
		void AddAxisAtPoint(const glm::vec4& point, float scale = 1.0f);
		void AddBox(glm::vec3 center, glm::vec3 dimensions, glm::vec4 colour);
		void DrawFrustum(const class Frustum& f, glm::vec4 colour);
		void DrawBox(glm::vec3 bmin, glm::vec3 bmax, glm::vec4 colour, glm::mat4 boxTransform = glm::identity<glm::mat4>());
		void DrawSphere(glm::vec3 center, float radius, glm::vec4 colour, glm::mat4 transform = glm::identity<glm::mat4>());
		void DrawCapsule(float radius, float halfHeight, glm::vec4 colour, glm::mat4 transform = glm::identity<glm::mat4>());
		void DrawCylinder(float radius, float halfHeight, glm::vec4 colour, glm::mat4 transform = glm::identity<glm::mat4>());

		void PushToRenderer(Renderer&);
	private:
		bool CreateMesh();
		void PushLinesToMesh(Render::Mesh& target);
		void AddLinesInternal(const __m128* posBuffer, const __m128* colBuffer, uint32_t count);

		static const uint32_t c_maxLines = 1024 * 512;
		uint32_t m_currentLines = 0;
		uint32_t m_currentWriteMesh = 0;
		std::unique_ptr<glm::vec4[], std::function<void(glm::vec4*)>> m_posBuffer;
		std::unique_ptr<glm::vec4[], std::function<void(glm::vec4*)>> m_colBuffer;
		ShaderHandle m_shader;
		static const uint32_t c_meshBuffers = 2;
		std::unique_ptr<Render::Mesh> m_renderMesh[c_meshBuffers];		
	};
}