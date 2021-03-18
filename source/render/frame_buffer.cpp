#include "frame_buffer.h"
#include "utils.h"
#include "texture.h"
#include "texture_source.h"
#include <cassert>

namespace Render
{
	FrameBuffer::FrameBuffer(glm::ivec2 size)
		: m_dimensions(size)
	{
	}

	FrameBuffer::~FrameBuffer()
	{
		Destroy();
	}

	void FrameBuffer::Resolve(FrameBuffer& target)
	{
		assert(m_msaaSamples != 1);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fboHandle);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.GetHandle());
		glBlitFramebuffer(0, 0, m_dimensions.x, m_dimensions.y, 0, 0, target.m_dimensions.x, target.m_dimensions.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		SDE_RENDER_PROCESS_GL_ERRORS("glBlitFrameBuffer");
	}

	TextureSource::Antialiasing GetAAMode(int samples)
	{
		auto aaMode = TextureSource::Antialiasing::None;
		switch (samples)
		{
		case 2:
			aaMode = TextureSource::Antialiasing::MSAAx2;
			break;
		case 4:
			aaMode = TextureSource::Antialiasing::MSAAx4;
			break;
		}
		return aaMode;
	}

	bool FrameBuffer::AddColourAttachment(ColourAttachmentFormat format)
	{
		auto textureFormat = Render::TextureSource::Format::Unsupported;
		switch (format)
		{
		case RGBA_U8:
			textureFormat = Render::TextureSource::Format::RGBA8;
			break;
		case RGBA_F16:
			textureFormat = Render::TextureSource::Format::RGBAF16;
			break;
		default:
			assert(false);	// Unsupported
		}
		TextureSource ts(m_dimensions.x, m_dimensions.y, textureFormat);
		ts.SetAA(GetAAMode(m_msaaSamples));
		auto newTexture = std::make_unique<Render::Texture>();
		if (newTexture->Create(ts))
		{
			m_colourAttachments.push_back(std::move(newTexture));
			return true;
		}
		else
		{
			return false;
		}
	}

	bool FrameBuffer::AddDepthCube()
	{
		TextureSource ts(m_dimensions.x, m_dimensions.y, Render::TextureSource::Format::Depth32);
		ts.SetAA(GetAAMode(m_msaaSamples));
		ts.SetWrapMode(TextureSource::WrapMode::ClampToEdge, TextureSource::WrapMode::ClampToEdge);
		auto newTexture = std::make_unique<Render::Texture>();
		if (newTexture->CreateCubemap(ts))
		{
			m_depthStencil = std::move(newTexture);
			m_isCubemap = true;
			return true;
		}
		return true;
	}

	bool FrameBuffer::AddDepth()
	{
		TextureSource ts(m_dimensions.x, m_dimensions.y, Render::TextureSource::Format::Depth32);
		ts.UseNearestFiltering() = true;
		ts.SetAA(GetAAMode(m_msaaSamples));
		ts.SetWrapMode(TextureSource::WrapMode::ClampToEdge, TextureSource::WrapMode::ClampToEdge);
		auto newTexture = std::make_unique<Render::Texture>();
		if (newTexture->Create(ts))
		{
			m_depthStencil = std::move(newTexture);
			m_isCubemap = false;
			return true;
		}
		return false;
	}

	bool FrameBuffer::AddDepthStencil()
	{
		TextureSource ts(m_dimensions.x, m_dimensions.y, Render::TextureSource::Format::Depth24Stencil8);
		ts.SetAA(GetAAMode(m_msaaSamples));
		ts.SetWrapMode(TextureSource::WrapMode::ClampToEdge, TextureSource::WrapMode::ClampToEdge);
		auto newTexture = std::make_unique<Render::Texture>();
		if (newTexture->Create(ts))
		{
			m_depthStencil = std::move(newTexture);
			m_isCubemap = false;
			return true;
		}
		return false;
	}

	bool FrameBuffer::Create()
	{
		glCreateFramebuffers(1, &m_fboHandle);
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCreateFramebuffers");

		for (int c = 0; c < m_colourAttachments.size(); ++c)
		{
			glNamedFramebufferTexture(m_fboHandle, GL_COLOR_ATTACHMENT0 + c, m_colourAttachments[c]->GetHandle(), 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedFramebufferTexture");
		}

		if (m_depthStencil != nullptr)
		{
			glNamedFramebufferTexture(m_fboHandle, GL_DEPTH_ATTACHMENT, m_depthStencil->GetHandle(), 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedFramebufferTexture");
		}

		// an fbo MUST have a colour attachment, we can use binding 0 if we dont have any
		if (m_colourAttachments.size() == 0)
		{
			glNamedFramebufferDrawBuffer(m_fboHandle, 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedFramebufferDrawBuffer");
			glNamedFramebufferReadBuffer(m_fboHandle, 0);
			SDE_RENDER_PROCESS_GL_ERRORS_RET("glNamedFramebufferReadBuffer");
		}

		bool readyToGo = glCheckNamedFramebufferStatus(m_fboHandle, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
		SDE_RENDER_PROCESS_GL_ERRORS_RET("glCheckNamedFramebufferStatus");
		return readyToGo;
	}

	void FrameBuffer::Destroy()
	{
		if (m_fboHandle != 0)
		{
			glDeleteFramebuffers(1, &m_fboHandle);
			SDE_RENDER_PROCESS_GL_ERRORS("glDeleteFramebuffers");
		}

		m_colourAttachments.clear();
	}
}