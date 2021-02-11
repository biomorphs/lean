#pragma once
#include "core/glm_headers.h"
#include <vector>
#include <memory>

namespace Render
{
	class Texture;
	class FrameBuffer
	{
	public:
		FrameBuffer(glm::ivec2 size);
		~FrameBuffer();

		enum ColourAttachmentFormat
		{
			RGBA_U8,
			RGBA_F16
		};

		bool AddColourAttachment(ColourAttachmentFormat format = RGBA_U8);
		bool AddDepthStencil();
		bool AddDepth();	// no stencil
		bool AddDepthCube();
		bool Create();
		void Destroy();

		uint32_t GetHandle() const { return m_fboHandle; }
		int GetColourAttachmentCount() const { return (int)m_colourAttachments.size(); }
		inline Texture& GetColourAttachment(int index) { return *m_colourAttachments[index]; }
		inline const Texture& GetColourAttachment(int index) const { return *m_colourAttachments[index]; }
		const std::unique_ptr<Texture>& GetDepthStencil() const { return m_depthStencil; }
		glm::ivec2 Dimensions() const {	return m_dimensions; }
		bool IsCubemap() const { return m_isCubemap; }

	private:
		std::vector<std::unique_ptr<Texture>> m_colourAttachments;
		std::unique_ptr<Texture> m_depthStencil;
		glm::ivec2 m_dimensions;
		bool m_isCubemap = false;
		uint32_t m_fboHandle = 0;
	};
}