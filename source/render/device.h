#pragma once

#include "core/glm_headers.h"
#include "render/fence.h"
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

	enum class ComputeImageFormat
	{
		R8,		// unsigned
		R8I,	// signed
		R32UI,
		RF16,
		RF32,
		RGBAF32,
		RGBAU8
	};

	enum class ComputeImageAccess
	{
		ReadOnly,
		WriteOnly,
		ReadWrite
	};

	enum class BarrierType
	{
		Image,
		TextureFetch,
		BufferData,
		VertexData,
		ShaderStorage,
		All
	};

	enum class FenceResult
	{
		Signalled,
		Timeout,
		Error
	};

	enum class DepthFunction
	{
		Less,		// write if depth < stored value
		LessOrEqual,// write if depth <= stored value
		Always		// always write
	};

	// Indirect rendering (allows us to batch draw calls!)
	// Build a RenderBuffer and pass it to DrawIndirect
	struct DrawIndirectIndexedParams
	{
		uint32_t m_indexCount;
		uint32_t m_instanceCount;
		uint32_t m_firstIndex;
		int m_baseVertex;
		uint32_t m_baseInstance;
	};

	// This represents the GL context for a window
	class Device
	{
	public:
		Device(Window& theWindow, bool makeDebugContext);
		~Device();
		Fence MakeFence();
		void DestroyFence(Fence& f);
		FenceResult WaitOnFence(Fence& f, uint32_t timeoutNanoseconds);		// destroys the fence on completion
		void WaitForGpu();	// gross
		void Present();
		void* CreateSharedGLContext();
		void* GetGLContext();
		static void FlushContext();
		void SetWireframeDrawing(bool wireframe);
		void SetGLContext(void* context);	// Sets context PER THREAD
		void SetViewport(glm::ivec2 pos, glm::ivec2 size);
		void SetScissorEnabled(bool enabled);
		void SetBlending(bool enabled);
		void SetBackfaceCulling(bool enabled, bool frontFaceCCW);
		void SetFrontfaceCulling(bool enabled, bool frontFaceCCW);
		void SetDepthState(bool enabled, bool writeEnabled);
		void SetDepthFunction(DepthFunction fn);
		void SetColourWriteMask(bool r, bool g, bool b, bool a);
		void ClearColourDepthTarget(const glm::vec4& colour, float depth);
		void ClearFramebufferColourDepth(const FrameBuffer& fb, const glm::vec4& colour, float depth);
		void ClearFramebufferColour(const FrameBuffer& fb, const glm::vec4& colour);
		void ClearFramebufferColour(const FrameBuffer& fb, int attachmentIndex, const glm::vec4& colour);
		void ClearFramebufferDepth(const FrameBuffer& fb, float depth);
		void DrawToFramebuffer(const FrameBuffer& fb);
		void DrawToFramebuffer(const FrameBuffer& fb, uint32_t cubeFace);
		void DrawToBackbuffer();
		void SetUniformValue(uint32_t uniformHandle, const glm::mat4& matrix);
		void SetUniformValue(uint32_t uniformHandle, const glm::vec4& val);
		void SetUniformValue(uint32_t uniformHandle, float val);
		void SetUniformValue(uint32_t uniformHandle, int32_t val);
		void SetUniformValue(uint32_t uniformHandle, uint32_t val);
		void MemoryBarrier(BarrierType m);
		void SetSampler(uint32_t uniformHandle, uint64_t residentHandle);
		void BindComputeImage(uint32_t bindIndex, uint32_t textureHandle, ComputeImageFormat f, ComputeImageAccess access, bool is3dOrArray=false);
		void DispatchCompute(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);
		void BindShaderProgram(const ShaderProgram& program);
		void BindVertexArray(const VertexArray& srcArray);
		void BindIndexBuffer(const RenderBuffer& buffer);
		void BindInstanceBuffer(const RenderBuffer& buffer, int vertexLayoutSlot, int components, size_t offset = 0, size_t vectorCount=1);
		void BindStorageBuffer(uint32_t ssboBindingIndex, const RenderBuffer& ssbo);	// like uniforms, but writeable!
		void BindDrawIndirectBuffer(const RenderBuffer& buffer);
		void DrawPrimitivesIndirectIndexed(PrimitiveType primitive, uint32_t startDrawCall, uint32_t drawCount);
		void DrawPrimitives(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount);
		void DrawPrimitivesInstancedIndexed(PrimitiveType primitive, uint32_t indexStart, uint32_t indexCount, uint32_t instanceCount, uint32_t firstInstance = 0);
		void DrawPrimitivesInstanced(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance=0);
		void BindUniformBufferIndex(ShaderProgram& p, const char* bufferName, uint32_t bindingIndex);
		void SetUniforms(ShaderProgram& p, const RenderBuffer& ubo, uint32_t uboBindingIndex);
		
	private:
		uint32_t TranslatePrimitiveType(PrimitiveType type) const;

		Window& m_window;
		void* m_context;
	};
}