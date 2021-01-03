/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "core/glm_headers.h"
#include <stdint.h>

namespace Render
{
	class Window;
	class Texture;
	class VertexArray;
	class ShaderProgram;
	class RenderBuffer;
	class FrameBuffer;

	enum class PrimitiveType : uint32_t
	{
		Triangles,
		Lines,
		PointSprites
	};

	// This represents the GL context for a window
	class Device
	{
	public:
		Device(Window& theWindow);
		~Device();
		void Present();
		void* CreateSharedGLContext();
		void* GetGLContext();
		static void FlushContext();
		void SetGLContext(void* context);	// Sets context PER THREAD
		void SetViewport(glm::ivec2 pos, glm::ivec2 size);
		void SetScissorEnabled(bool enabled);
		void SetBlending(bool enabled);
		void SetBackfaceCulling(bool enabled, bool frontFaceCCW);
		void SetFrontfaceCulling(bool enabled, bool frontFaceCCW);
		void SetDepthState(bool enabled, bool writeEnabled);
		void ClearColourDepthTarget(const glm::vec4& colour, float depth);
		void ClearFramebufferColourDepth(const FrameBuffer& fb, const glm::vec4& colour, float depth);
		void ClearFramebufferDepth(const FrameBuffer& fb, float depth);
		void DrawToFramebuffer(const FrameBuffer& fb);
		void DrawToFramebuffer(const FrameBuffer& fb, uint32_t cubeFace);
		void DrawToBackbuffer();
		void SetUniformValue(uint32_t uniformHandle, const glm::mat4& matrix);
		void SetUniformValue(uint32_t uniformHandle, const glm::vec4& val);
		void SetUniformValue(uint32_t uniformHandle, float val);
		void SetUniformValue(uint32_t uniformHandle, int32_t val);
		void SetSampler(uint32_t uniformHandle, uint32_t textureHandle, uint32_t textureUnit);
		void SetArraySampler(uint32_t uniformHandle, uint32_t textureHandle, uint32_t textureUnit);
		void BindShaderProgram(const ShaderProgram& program);
		void BindVertexArray(const VertexArray& srcArray);
		void BindInstanceBuffer(const VertexArray& srcArray, const RenderBuffer& buffer, int vertexLayoutSlot, int components, size_t offset = 0, size_t vectorCount=1);
		void DrawPrimitives(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount);
		void DrawPrimitivesInstanced(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance=0);
		void BindUniformBufferIndex(ShaderProgram& p, const char* bufferName, uint32_t bindingIndex);
		void SetUniforms(ShaderProgram& p, const RenderBuffer& ubo, uint32_t uboBindingIndex);
	private:
		uint32_t TranslatePrimitiveType(PrimitiveType type) const;

		Window& m_window;
		void* m_context;
	};
}