#include "device.h"
#include "window.h"
#include "texture.h"
#include "utils.h"
#include "vertex_array.h"
#include "shader_program.h"
#include "render_buffer.h"
#include "frame_buffer.h"
#include "core/glm_headers.h"
#include "core/profiler.h"
#include <SDL.h>
#include <GL/glew.h>

namespace Render
{
	void GLDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message,const void* user)
	{
		SDE_LOG("OpenGL is telling us something!\n\t(%d): %s",id, message);

		char* sourceStr = "unknown";
		switch (source)
		{
			case GL_DEBUG_SOURCE_API:             sourceStr = "API";	break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Window System";	break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler";	break;
			case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Third Party";	break;
			case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Application";	break;
			case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Other";	break;
		}

		char* typeStr = "unknown";
		switch (type)
		{
			case GL_DEBUG_TYPE_ERROR:              typeStr = "Error";		break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:typeStr = "Deprecated Behaviour";	break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "Undefined Behaviour";	break;
			case GL_DEBUG_TYPE_PORTABILITY:        typeStr = "Portability";	break;
			case GL_DEBUG_TYPE_PERFORMANCE:        typeStr = "Performance";	break;
			case GL_DEBUG_TYPE_MARKER:             typeStr = "Marker";	break;
			case GL_DEBUG_TYPE_PUSH_GROUP:         typeStr = "Push Group";	break;
			case GL_DEBUG_TYPE_POP_GROUP:          typeStr = "Pop Group";		break;
			case GL_DEBUG_TYPE_OTHER:              typeStr = "Other";		break;
		}
		char* severityStr = "unknown";
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         severityStr = "High";	break;
			case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "Medium";	break;
			case GL_DEBUG_SEVERITY_LOW:          severityStr = "Low";	break;
			case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "Notification";	break;
		} 
		SDE_LOG("\tSource: %s\n\tType: %s\n\tSeverity: %s", sourceStr, typeStr, severityStr);
		//__debugbreak();
	}

	Device::Device(Window& theWindow, bool makeDebugContext)
		: m_window( theWindow )
	{
		SDE_PROF_EVENT();

		SDL_Window* windowHandle = theWindow.GetWindowHandle();
		assert(windowHandle);

		m_context = SDL_GL_CreateContext(windowHandle);
		assert(m_context);

		// glew initialises GL function pointers
		glewExperimental = true;		// must be set for core profile and above
		auto glewError = glewInit();
		assert(glewError == GLEW_OK);

		// glewInit can push INVALID_ENUM to the error stack (due to how extensions are checked)
		// pop everything off here
		auto glErrorPop = glGetError();
		while (glErrorPop == GL_INVALID_ENUM)
		{
			glErrorPop = glGetError();
		}

		int flags=0; 
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT && makeDebugContext)
		{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(GLDebugOutput, nullptr);

			// enable all messages
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

			// but disable low priority stuff
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);

			// we also dont care about notifications
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		}

		// Setting this here allows all point sprite shaders to set the sprite size
		// dynamically.
		glEnable(GL_PROGRAM_POINT_SIZE);
	}

	Device::~Device()
	{
		SDL_GL_DeleteContext(m_context);
		m_context = nullptr;
	}

	void Device::DestroyFence(Fence& f)
	{
		SDE_PROF_EVENT();

		if (f.m_data)
		{
			glDeleteSync((GLsync)f.m_data);
			f.m_data = nullptr;
		}
	}

	FenceResult Device::WaitOnFence(Fence& f, uint32_t timeoutNanoseconds)
	{
		SDE_PROF_EVENT();

		auto result = glClientWaitSync((GLsync)f.m_data, 0, timeoutNanoseconds);

		if (result != GL_TIMEOUT_EXPIRED)
		{
			DestroyFence(f);
		}

		switch (result)
		{
		case GL_CONDITION_SATISFIED:
		case GL_ALREADY_SIGNALED:
			return FenceResult::Signalled;
		case GL_TIMEOUT_EXPIRED:
			return FenceResult::Timeout;
		default:
			return FenceResult::Error;
		}
	}

	Fence Device::MakeFence()
	{
		auto result = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		return Fence(result);
	}

	void Device::MemoryBarrier(BarrierType m)
	{
		uint32_t barrierMode = 0;
		switch (m)
		{
		case BarrierType::Image:
			barrierMode = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
			break;
		case BarrierType::TextureFetch:
			barrierMode = GL_TEXTURE_FETCH_BARRIER_BIT;
			break;
		case BarrierType::BufferData:
			barrierMode = GL_BUFFER_UPDATE_BARRIER_BIT;
			break;
		case BarrierType::VertexData:
			barrierMode = GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
			break;
		case BarrierType::ShaderStorage:
			barrierMode = GL_SHADER_STORAGE_BARRIER_BIT;
			break;
		case BarrierType::All:
			barrierMode = GL_ALL_BARRIER_BITS;
			break;
		default:
			assert(!"Whut");
		}
		glMemoryBarrier(barrierMode);
	}

	void Device::SetWireframeDrawing(bool wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
	}

	void Device::SetViewport(glm::ivec2 pos, glm::ivec2 size)
	{
		glViewport(pos.x, pos.y, size.x, size.y);
	}

	void Device::ClearFramebufferDepth(const FrameBuffer& fb, float depth)
	{
		if (fb.GetDepthStencil() != nullptr)
		{
			glClearNamedFramebufferfv(fb.GetHandle(), GL_DEPTH, 0, &depth);
		}
	}

	void Device::ClearFramebufferColour(const FrameBuffer& fb, const glm::vec4& colour)
	{
		int colourAttachments = fb.GetColourAttachmentCount();
		for (int i = 0; i < colourAttachments; ++i)
		{
			glClearNamedFramebufferfv(fb.GetHandle(), GL_COLOR, i, glm::value_ptr(colour));
		}
	}

	void Device::ClearFramebufferColourDepth(const FrameBuffer& fb, const glm::vec4& colour, float depth)
	{
		int colourAttachments= fb.GetColourAttachmentCount();
		for (int i = 0; i < colourAttachments; ++i)
		{
			glClearNamedFramebufferfv(fb.GetHandle(), GL_COLOR, i, glm::value_ptr(colour));
		}

		if (fb.GetDepthStencil() != nullptr)
		{
			glClearNamedFramebufferfv(fb.GetHandle(), GL_DEPTH, 0, &depth);
		}
	}

	void Device::DrawToBackbuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Device::DrawToFramebuffer(const FrameBuffer& fb)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb.GetHandle());
	}

	void Device::DrawToFramebuffer(const FrameBuffer& fb, uint32_t cubeFace)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb.GetHandle());
		glNamedFramebufferTextureLayer(fb.GetHandle(), GL_DEPTH_ATTACHMENT, fb.GetDepthStencil()->GetHandle(), 0, cubeFace);
	}

	void Device::FlushContext()
	{
		glFlush();	// Ensures any writes in shared contexts are pushed to all of them
	}

	void Device::SetGLContext(void* context)
	{
		SDL_GL_MakeCurrent(m_window.GetWindowHandle(), context);
	}

	void* Device::CreateSharedGLContext()
	{
		auto newContext = SDL_GL_CreateContext(m_window.GetWindowHandle());
		return newContext;
	}

	void Device::Present()
	{
		SDE_PROF_EVENT();
		SDL_GL_SwapWindow(m_window.GetWindowHandle());
		glFinish();
	}

	SDL_GLContext Device::GetGLContext()
	{
		return m_context;
	}

	inline uint32_t Device::TranslatePrimitiveType(PrimitiveType type) const
	{
		switch (type)
		{
		case PrimitiveType::Triangles:
			return GL_TRIANGLES;
		case PrimitiveType::Lines:
			return GL_LINES;
		case PrimitiveType::PointSprites:
			return GL_POINTS;
		default:
			return -1;
		}
	}

	void Device::SetScissorEnabled(bool enabled)
	{
		if (enabled)
		{
			glEnable(GL_SCISSOR_TEST);
		}
		else
		{
			glDisable(GL_SCISSOR_TEST);
		}
	}

	void Device::SetBlending(bool enabled)
	{
		if (enabled)
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	void Device::SetFrontfaceCulling(bool enabled, bool frontFaceCCW)
	{
		if (enabled)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}

		glFrontFace(frontFaceCCW ? GL_CCW : GL_CW);
	}

	void Device::SetBackfaceCulling(bool enabled, bool frontFaceCCW)
	{
		if (enabled)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}

		glFrontFace(frontFaceCCW ? GL_CCW : GL_CW);
	}

	void Device::SetDepthState(bool enabled, bool writeEnabled)
	{
		if (enabled)
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
		glDepthMask(writeEnabled);
	}

	void Device::ClearColourDepthTarget(const glm::vec4& colour, float depth)
	{
		SDE_PROF_EVENT();
		glClearColor(colour.r, colour.g, colour.b, colour.a);
		glClearDepth(depth);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Device::SetSampler(uint32_t uniformHandle, uint64_t residentHandle)
	{
		assert(uniformHandle != -1);
		glUniformHandleui64ARB(uniformHandle, residentHandle);
	}

	void Device::SetUniformValue(uint32_t uniformHandle, const glm::mat4& matrix)
	{
		assert(uniformHandle != -1);
		glUniformMatrix4fv(uniformHandle, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void Device::SetUniformValue(uint32_t uniformHandle, const glm::vec4& val)
	{
		assert(uniformHandle != -1);
		glUniform4fv(uniformHandle, 1, glm::value_ptr(val));
	}

	void Device::SetUniformValue(uint32_t uniformHandle, float val)
	{
		assert(uniformHandle != -1);
		glUniform1f(uniformHandle, val);
	}

	void Device::SetUniformValue(uint32_t uniformHandle, int32_t val)
	{
		assert(uniformHandle != -1);
		glUniform1i(uniformHandle, val);
	}

	void Device::SetUniformValue(uint32_t uniformHandle, uint32_t val)
	{
		assert(uniformHandle != -1);
		glUniform1ui(uniformHandle, val);
	}

	void Device::DispatchCompute(uint32_t groupsX, uint32_t groupsY = 0, uint32_t groupsZ = 0)
	{
		glDispatchCompute(groupsX, groupsY, groupsZ);
	}

	void Device::BindComputeImage(uint32_t bindIndex, uint32_t textureHandle, ComputeImageFormat f, ComputeImageAccess access, bool is3dOrArray)
	{
		uint32_t accessMode = 0;
		uint32_t format = 0;
		switch (f)
		{
		case ComputeImageFormat::RGBAF32:
			format = GL_RGBA32F;
			break;
		case ComputeImageFormat::R8:
			format = GL_R8;
			break;
		case ComputeImageFormat::R8I:
			format = GL_R8I;
			break;
		case ComputeImageFormat::RF16:
			format = GL_R16F;
			break;
		case ComputeImageFormat::RF32:
			format = GL_R32F;
			break;
		case ComputeImageFormat::R32UI:
			format = GL_R32UI;
			break;
		default:
			assert(!"Whut");
		}
		switch (access)
		{
		case ComputeImageAccess::ReadOnly:
			accessMode = GL_READ_ONLY;
			break;
		case ComputeImageAccess::WriteOnly:
			accessMode = GL_WRITE_ONLY;
			break;
		case ComputeImageAccess::ReadWrite:
			accessMode = GL_READ_WRITE;
			break;
		default:
			assert(!"Whut");
		}
		glBindImageTexture(bindIndex, textureHandle, 0, is3dOrArray ? GL_TRUE : GL_FALSE, 0, accessMode, format);
	}

	void Device::BindShaderProgram(const ShaderProgram& program)
	{
		glUseProgram(program.GetHandle());
	}

	// vectorcount used to pass matrices (4x4 mat = 4 components, 4 vectorcount)
	void Device::BindInstanceBuffer(const RenderBuffer& buffer, int vertexLayoutSlot, int components, size_t offset, size_t vectorCount)
	{
		assert(buffer.GetHandle() != 0);
		assert(components <= 4);

		glBindBuffer(GL_ARRAY_BUFFER, buffer.GetHandle());		// bind the vbo

		glEnableVertexAttribArray(vertexLayoutSlot);			//enable the slot

		// send the data (we have to send it 4 components at a time)
		// always float, never normalised
		glVertexAttribPointer(vertexLayoutSlot, components, GL_FLOAT, GL_FALSE, components * sizeof(float) * (int)vectorCount, (void*)offset);

		glVertexAttribDivisor(vertexLayoutSlot, 1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void Device::BindDrawIndirectBuffer(const RenderBuffer& buffer)
	{
		assert(buffer.GetHandle() != 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer.GetHandle());
	}

	void Device::BindIndexBuffer(const RenderBuffer& buffer)
	{
		assert(buffer.GetHandle() != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.GetHandle());
	}

	void Device::BindVertexArray(const VertexArray& srcArray)
	{
		assert(srcArray.GetHandle() != 0);
		glBindVertexArray(srcArray.GetHandle());
	}

	void Device::DrawPrimitivesIndirectIndexed(PrimitiveType primitive, uint32_t startDrawCall, uint32_t drawCount)
	{
		SDE_PROF_EVENT();

		auto primitiveType = TranslatePrimitiveType(primitive);
		assert(primitiveType != -1);
		const void* offsetPtr = (void*)(startDrawCall * sizeof(DrawIndirectIndexedParams));
		glMultiDrawElementsIndirect(primitiveType, GL_UNSIGNED_INT, offsetPtr, drawCount, 0);
	}

	void Device::DrawPrimitivesInstancedIndexed(PrimitiveType primitive, uint32_t indexStart, uint32_t indexCount, uint32_t instanceCount, uint32_t firstInstance)
	{
		SDE_PROF_EVENT();

		auto primitiveType = TranslatePrimitiveType(primitive);
		assert(primitiveType != -1);

		// this is confusing as hell, but the 4th param is not a pointer, but an OFFSET into the element (index) buffer
		const void* indexDataPtr = (void*)(indexStart * sizeof(uint32_t));
		glDrawElementsInstancedBaseVertexBaseInstance(primitiveType, indexCount, GL_UNSIGNED_INT, indexDataPtr, instanceCount, 0, firstInstance);
	}

	void Device::DrawPrimitivesInstanced(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance)
	{
		SDE_PROF_EVENT();

		auto primitiveType = TranslatePrimitiveType(primitive);
		assert(primitiveType != -1);

		glDrawArraysInstancedBaseInstance(primitiveType, vertexStart, vertexCount, instanceCount, firstInstance);
	}

	void Device::DrawPrimitives(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount)
	{
		auto primitiveType = TranslatePrimitiveType(primitive);
		assert(primitiveType != -1);

		glDrawArrays(primitiveType, vertexStart, vertexCount);
	}

	void Device::BindStorageBuffer(uint32_t ssboBindingIndex, const RenderBuffer& ssbo)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssboBindingIndex, ssbo.GetHandle());
	}

	void Device::SetUniforms(ShaderProgram& p, const RenderBuffer& ubo, uint32_t uboBindingIndex)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, uboBindingIndex, ubo.GetHandle());
	}

	void Device::BindUniformBufferIndex(ShaderProgram& p, const char* bufferName, uint32_t bindingIndex)
	{
		// First we find the uniform block index
		uint32_t blockIndex = p.GetUniformBufferBlockIndex(bufferName);
		if (blockIndex != GL_INVALID_INDEX)
		{
			// create a binding between the uniforms in the shader and the global ubo array (bindingIndex)
			glUniformBlockBinding(p.GetHandle(), blockIndex, bindingIndex);
		}
	}
}