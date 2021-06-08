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
	Device::Device(Window& theWindow)
		: m_window( theWindow )
	{
		SDE_PROF_EVENT();

		SDL_Window* windowHandle = theWindow.GetWindowHandle();
		SDE_RENDER_ASSERT(windowHandle);

		m_context = SDL_GL_CreateContext(windowHandle);
		SDE_RENDER_ASSERT(m_context);

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
		SDE_RENDER_PROCESS_GL_ERRORS("Device Initialise");

		// Setting this here allows all point sprite shaders to set the sprite size
		// dynamically.
		glEnable(GL_PROGRAM_POINT_SIZE);
	}

	Device::~Device()
	{
		SDL_GL_DeleteContext(m_context);
		m_context = nullptr;
	}

	void Device::MemoryBarrier(BarrierType m)
	{
		uint32_t barrierMode = 0;
		switch (m)
		{
		case BarrierType::Image:
			barrierMode = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
			break;
		default:
			assert(!"Whut");
		}
		glMemoryBarrier(barrierMode);
		SDE_RENDER_PROCESS_GL_ERRORS("glMemoryBarrier");
	}

	void Device::SetViewport(glm::ivec2 pos, glm::ivec2 size)
	{
		glViewport(pos.x, pos.y, size.x, size.y);
		SDE_RENDER_PROCESS_GL_ERRORS("glViewport");
	}

	void Device::ClearFramebufferDepth(const FrameBuffer& fb, float depth)
	{
		if (fb.GetDepthStencil() != nullptr)
		{
			glClearNamedFramebufferfi(fb.GetHandle(), GL_DEPTH_STENCIL, 0, depth, 0);
			SDE_RENDER_PROCESS_GL_ERRORS("glClearNamedFramebufferfi");
		}
	}

	void Device::ClearFramebufferColour(const FrameBuffer& fb, const glm::vec4& colour)
	{
		int colourAttachments = fb.GetColourAttachmentCount();
		for (int i = 0; i < colourAttachments; ++i)
		{
			glClearNamedFramebufferfv(fb.GetHandle(), GL_COLOR, i, glm::value_ptr(colour));
			SDE_RENDER_PROCESS_GL_ERRORS("glClearNamedFramebufferfv");
		}
	}

	void Device::ClearFramebufferColourDepth(const FrameBuffer& fb, const glm::vec4& colour, float depth)
	{
		int colourAttachments= fb.GetColourAttachmentCount();
		for (int i = 0; i < colourAttachments; ++i)
		{
			glClearNamedFramebufferfv(fb.GetHandle(), GL_COLOR, i, glm::value_ptr(colour));
			SDE_RENDER_PROCESS_GL_ERRORS("glClearNamedFramebufferfv");
		}

		if (fb.GetDepthStencil() != nullptr)
		{
			glClearNamedFramebufferfi(fb.GetHandle(), GL_DEPTH_STENCIL, 0, depth, 0);
			SDE_RENDER_PROCESS_GL_ERRORS("glClearNamedFramebufferfi");
		}
	}

	void Device::DrawToBackbuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindFramebuffer");
	}

	void Device::DrawToFramebuffer(const FrameBuffer& fb)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glBindFramebuffer");
	}

	void Device::DrawToFramebuffer(const FrameBuffer& fb, uint32_t cubeFace)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glBindFramebuffer");
		glNamedFramebufferTextureLayer(fb.GetHandle(), GL_DEPTH_ATTACHMENT, fb.GetDepthStencil()->GetHandle(), 0, cubeFace);
		SDE_RENDER_PROCESS_GL_ERRORS("glNamedFramebufferTextureLayer");
	}

	void Device::FlushContext()
	{
		glFlush();	// Ensures any writes in shared contexts are pushed to all of them
	}

	void Device::SetGLContext(void* context)
	{
		SDL_GL_MakeCurrent(m_window.GetWindowHandle(), context);
		SDE_RENDER_PROCESS_GL_ERRORS("SDL_GL_MakeCurrent");
	}

	void* Device::CreateSharedGLContext()
	{
		auto newContext = SDL_GL_CreateContext(m_window.GetWindowHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("SDL_GL_CreateContext");
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
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
			glBlendEquation(GL_FUNC_ADD);
			SDE_RENDER_PROCESS_GL_ERRORS("glBlendEquation");
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			SDE_RENDER_PROCESS_GL_ERRORS("glBlendFunc");
		}
		else
		{
			glDisable(GL_BLEND);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}
	}

	void Device::SetFrontfaceCulling(bool enabled, bool frontFaceCCW)
	{
		if (enabled)
		{
			glEnable(GL_CULL_FACE);
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
			glCullFace(GL_FRONT);
			SDE_RENDER_PROCESS_GL_ERRORS("glCullFace");
		}
		else
		{
			glDisable(GL_CULL_FACE);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}

		glFrontFace(frontFaceCCW ? GL_CCW : GL_CW);
		SDE_RENDER_PROCESS_GL_ERRORS("glFrontFace");
	}

	void Device::SetBackfaceCulling(bool enabled, bool frontFaceCCW)
	{
		if (enabled)
		{
			glEnable(GL_CULL_FACE);
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
			glCullFace(GL_BACK);
			SDE_RENDER_PROCESS_GL_ERRORS("glCullFace");
		}
		else
		{
			glDisable(GL_CULL_FACE);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}

		glFrontFace(frontFaceCCW ? GL_CCW : GL_CW);
		SDE_RENDER_PROCESS_GL_ERRORS("glFrontFace");
	}

	void Device::SetDepthState(bool enabled, bool writeEnabled)
	{
		if (enabled)
		{
			glEnable(GL_DEPTH_TEST);
			SDE_RENDER_PROCESS_GL_ERRORS("glEnable");
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
			SDE_RENDER_PROCESS_GL_ERRORS("glDisable");
		}
		glDepthMask(writeEnabled);
		SDE_RENDER_PROCESS_GL_ERRORS("glDepthMask");
	}

	void Device::ClearColourDepthTarget(const glm::vec4& colour, float depth)
	{
		SDE_PROF_EVENT();
		glClearColor(colour.r, colour.g, colour.b, colour.a);
		glClearDepth(depth);
		SDE_RENDER_PROCESS_GL_ERRORS("glClearColor");
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDE_RENDER_PROCESS_GL_ERRORS("glClear");
	}

	void Device::SetArraySampler(uint32_t uniformHandle, uint32_t textureHandle, uint32_t textureUnit)
	{
		assert(uniformHandle != -1);
		assert(textureHandle != 0);

		glBindTextureUnit(textureUnit, textureHandle);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindTextureUnit");

		glUniform1i(uniformHandle, textureUnit);
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform1i");
	}

	void Device::SetSampler(uint32_t uniformHandle, uint32_t textureHandle, uint32_t textureUnit)
	{
		assert(uniformHandle != -1);

		glBindTextureUnit(textureUnit, textureHandle);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindTextureUnit");

		glUniform1i(uniformHandle, textureUnit);
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform1i");
	}

	void Device::SetUniformValue(uint32_t uniformHandle, const glm::mat4& matrix)
	{
		assert(uniformHandle != -1);
		glUniformMatrix4fv(uniformHandle, 1, GL_FALSE, glm::value_ptr(matrix));
		SDE_RENDER_PROCESS_GL_ERRORS("glUniformMatrix4fv");
	}

	void Device::SetUniformValue(uint32_t uniformHandle, const glm::vec4& val)
	{
		assert(uniformHandle != -1);
		glUniform4fv(uniformHandle, 1, glm::value_ptr(val));
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform4fv");
	}

	void Device::SetUniformValue(uint32_t uniformHandle, float val)
	{
		assert(uniformHandle != -1);
		glUniform1f(uniformHandle, val);
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform1f");
	}

	void Device::SetUniformValue(uint32_t uniformHandle, int32_t val)
	{
		assert(uniformHandle != -1);
		glUniform1i(uniformHandle, val);
		SDE_RENDER_PROCESS_GL_ERRORS("glUniform1i");
	}

	void Device::DispatchCompute(uint32_t groupsX, uint32_t groupsY = 0, uint32_t groupsZ = 0)
	{
		glDispatchCompute(groupsX, groupsY, groupsZ);
		SDE_RENDER_PROCESS_GL_ERRORS("glDispatchCompute");
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
		SDE_RENDER_PROCESS_GL_ERRORS("glBindImageTexture");
	}

	void Device::BindShaderProgram(const ShaderProgram& program)
	{
		glUseProgram(program.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glUseProgram");
	}

	// vectorcount used to pass matrices (4x4 mat = 4 components, 4 vectorcount)
	void Device::BindInstanceBuffer(const VertexArray& srcArray, const RenderBuffer& buffer, int vertexLayoutSlot, int components, size_t offset, size_t vectorCount)
	{
		assert(buffer.GetHandle() != 0);
		assert(components <= 4);

		BindVertexArray(srcArray);

		glBindBuffer(GL_ARRAY_BUFFER, buffer.GetHandle());		// bind the vbo
		SDE_RENDER_PROCESS_GL_ERRORS("glBindBuffer");

		glEnableVertexAttribArray(vertexLayoutSlot);			//enable the slot
		SDE_RENDER_PROCESS_GL_ERRORS("glEnableVertexAttribArray");

		// send the data (we have to send it 4 components at a time)
		// always float, never normalised
		glVertexAttribPointer(vertexLayoutSlot, components, GL_FLOAT, GL_FALSE, components * sizeof(float) * (int)vectorCount, (void*)offset);
		SDE_RENDER_PROCESS_GL_ERRORS("glVertexAttribPointer");

		glVertexAttribDivisor(vertexLayoutSlot, 1);
		SDE_RENDER_PROCESS_GL_ERRORS("glVertexAttribDivisor");

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		SDE_RENDER_PROCESS_GL_ERRORS("glBindBuffer");
	}

	void Device::BindVertexArray(const VertexArray& srcArray)
	{
		assert(srcArray.GetHandle() != 0);
		glBindVertexArray(srcArray.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glBindVertexArray");
	}

	void Device::DrawPrimitivesInstanced(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance)
	{
		SDE_PROF_EVENT();

		auto primitiveType = TranslatePrimitiveType(primitive);
		assert(primitiveType != -1);

		glDrawArraysInstancedBaseInstance(primitiveType, vertexStart, vertexCount, instanceCount, firstInstance);
		SDE_RENDER_PROCESS_GL_ERRORS("glDrawArraysInstanced");
	}

	void Device::DrawPrimitives(PrimitiveType primitive, uint32_t vertexStart, uint32_t vertexCount)
	{
		auto primitiveType = TranslatePrimitiveType(primitive);
		assert(primitiveType != -1);

		glDrawArrays(primitiveType, vertexStart, vertexCount);
		SDE_RENDER_PROCESS_GL_ERRORS("glDrawArrays");
	}

	void Device::SetStorageBuffer(ShaderProgram& p, const RenderBuffer& ssbo, uint32_t ssboBindingIndex)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssboBindingIndex, ssbo.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glBindBufferBase");
	}

	void Device::SetUniforms(ShaderProgram& p, const RenderBuffer& ubo, uint32_t uboBindingIndex)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, uboBindingIndex, ubo.GetHandle());
		SDE_RENDER_PROCESS_GL_ERRORS("glBindBufferBase");
	}

	void Device::BindUniformBufferIndex(ShaderProgram& p, const char* bufferName, uint32_t bindingIndex)
	{
		// First we find the uniform block index
		uint32_t blockIndex = p.GetUniformBufferBlockIndex(bufferName);
		if (blockIndex != GL_INVALID_INDEX)
		{
			// create a binding between the uniforms in the shader and the global ubo array (bindingIndex)
			glUniformBlockBinding(p.GetHandle(), blockIndex, bindingIndex);
			SDE_RENDER_PROCESS_GL_ERRORS("glUniformBlockBinding");
		}
	}
}