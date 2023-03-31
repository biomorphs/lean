#include "texture.h"
#include "texture_source.h"
#include "utils.h"
#include "core/profiler.h"
#include <algorithm>

namespace Render
{
	Texture::Texture()
	{
	}

	Texture::~Texture()
	{
		Destroy();
	}

	uint32_t SourceFormatToComponentCount(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::DXT1:
			return 3;
		case TextureSource::Format::DXT3:
			return 4;
		case TextureSource::Format::DXT5:
			return 4;
		case TextureSource::Format::RGBA8:
			return 4;
		case TextureSource::Format::RGB8:
			return 3;
		case TextureSource::Format::R8:
		case TextureSource::Format::R8I:
		case TextureSource::Format::RF16:
		case TextureSource::Format::RF32:
		case TextureSource::Format::R32UI:
			return 1;
		case TextureSource::Format::RGBAF16:
			return 4;
		case TextureSource::Format::RGBAF32:
			return 4;
		case TextureSource::Format::Depth24Stencil8:
			return 1;
		case TextureSource::Format::Depth32:
			return 1;
		default:
			return -1;
		}
	}

	uint32_t SourceFormatToGLStorageFormat(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::DXT1:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case TextureSource::Format::DXT3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case TextureSource::Format::DXT5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TextureSource::Format::RGBA8:
			return GL_RGBA8;
		case TextureSource::Format::RGB8:
			return GL_RGB8;
		case TextureSource::Format::R8:
			return GL_R8;
		case TextureSource::Format::R8I:
			return GL_R8I;
		case TextureSource::Format::R32UI:
			return GL_R32UI;
		case TextureSource::Format::RF16:
			return GL_R16F;
		case TextureSource::Format::RF32:
			return GL_R32F;
		case TextureSource::Format::RGBAF16:
			return GL_RGBA16F;
		case TextureSource::Format::RGBAF32:
			return GL_RGBA32F;
		case TextureSource::Format::Depth24Stencil8:
			return GL_DEPTH24_STENCIL8;
		case TextureSource::Format::Depth32:
			return GL_DEPTH_COMPONENT32F;
		default:
			return -1;
		}
	}

	uint32_t SourceFormatToGLInternalType(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::R8I:
			return GL_BYTE;
		case TextureSource::Format::R8:
		case TextureSource::Format::RGB8:
		case TextureSource::Format::RGBA8:
			return GL_UNSIGNED_BYTE;
		case TextureSource::Format::RF16:
			return GL_HALF_FLOAT;
		case TextureSource::Format::RF32:
		case TextureSource::Format::RGBAF32:
			return GL_FLOAT;
		default:
			return -1;
		}
	}

	uint32_t SourceFormatToGLInternalFormat(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::R8:
			return GL_RED;
		case TextureSource::Format::RGB8:
			return GL_RGB;
		case TextureSource::Format::RGBA8:
		case TextureSource::Format::RGBAF32:
			return GL_RGBA;
		default:
			return -1;
		}
	}

	bool ShouldCreateCompressed(TextureSource::Format f)
	{
		switch (f)
		{
		case TextureSource::Format::DXT1:
			return true;
		case TextureSource::Format::DXT3:
			return true;
		case TextureSource::Format::DXT5:
			return true;
		default:
			return false;
		}
	}

	uint32_t GetGeneratedMipCount(const TextureSource& src)
	{
		uint32_t mipCount = 0;
		uint32_t smallestDim = std::min(src.Width(), src.Height());
		while (smallestDim > 1)
		{
			smallestDim = smallestDim >> 1;
			mipCount++;
		}
		return mipCount;
	}

	uint32_t WrapModeToGlType(TextureSource::WrapMode m)
	{
		switch (m)
		{
		case TextureSource::WrapMode::ClampToEdge:
			return GL_CLAMP_TO_EDGE;
		case TextureSource::WrapMode::Repeat:
			return GL_REPEAT;
		default:
			assert(false);
			return GL_REPEAT;
		}
	}

	void Texture::SetClampToBorder(glm::vec4 borderColour)
	{
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTextureParameterfv(m_handle, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(borderColour));
	}

	bool Texture::CreateSimpleUncompressedTexture3D(const TextureSource& src)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(src.GetAA() == TextureSource::Antialiasing::None);
		SDE_RENDER_ASSERT(src.Depth() > 0);

		glCreateTextures(GL_TEXTURE_3D, 1, &m_handle);

		m_componentCount = SourceFormatToComponentCount(src.SourceFormat());
		const bool shouldGenerateMips = src.MipCount() <= 1 && src.ShouldGenerateMips();
		const uint32_t mipCount = shouldGenerateMips ? GetGeneratedMipCount(src) : src.MipCount();

		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, WrapModeToGlType(src.GetWrapModeS()));
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, WrapModeToGlType(src.GetWrapModeT()));
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, WrapModeToGlType(src.GetWrapModeR()));

		auto filterMode = src.UseNearestFiltering() ? GL_NEAREST : GL_LINEAR;
		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, filterMode);
		if (mipCount > 1)
		{
			filterMode = src.UseNearestFiltering() ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, filterMode);
		}
		else
		{
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, filterMode);
		}

		uint32_t glStorageFormat = SourceFormatToGLStorageFormat(src.SourceFormat());
		SDE_RENDER_ASSERT(glStorageFormat != -1);
		{
			SDE_PROF_EVENT("AllocateStorage");
			// This preallocates the entire mip-chain
			glTextureStorage3D(m_handle, std::max(mipCount, 1u), glStorageFormat, src.Width(), src.Height(), src.Depth());
		}

		if (shouldGenerateMips)
		{
			SDE_PROF_EVENT("GenerateMips");
			glGenerateTextureMipmap(m_handle);
		}
		return m_handle != 0;
	}

	bool Texture::CreateSimpleUncompressedTexture2D(const TextureSource& src)
	{
		SDE_PROF_EVENT();

		uint32_t target = src.GetAA() == TextureSource::Antialiasing::None ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE;

		glCreateTextures(target, 1, &m_handle);

		m_componentCount = SourceFormatToComponentCount(src.SourceFormat());
		const bool shouldGenerateMips = src.MipCount() <=1 && src.ShouldGenerateMips();
		const uint32_t mipCount = shouldGenerateMips ? GetGeneratedMipCount(src) : src.MipCount();	

		if (src.GetAA() == TextureSource::Antialiasing::None)
		{
			glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, WrapModeToGlType(src.GetWrapModeS()));
			glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, WrapModeToGlType(src.GetWrapModeT()));

			auto filterMode = src.UseNearestFiltering() ? GL_NEAREST : GL_LINEAR;
			glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, filterMode);
			if (mipCount > 1)
			{
				filterMode = src.UseNearestFiltering() ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
				glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, filterMode);
			}
			else
			{
				glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, filterMode);
			}
		}

		uint32_t glStorageFormat = SourceFormatToGLStorageFormat(src.SourceFormat());
		SDE_RENDER_ASSERT(glStorageFormat != -1);
		{
			SDE_PROF_EVENT("AllocateStorage");
			// This preallocates the entire mip-chain
			if (src.GetAA() == TextureSource::Antialiasing::None)
			{
				glTextureStorage2D(m_handle, std::max(mipCount, 1u), glStorageFormat, src.Width(), src.Height());
			}
			else
			{
				uint32_t sampleCount = 2;
				switch (src.GetAA())
				{
				case TextureSource::Antialiasing::MSAAx4:
					sampleCount = 4;
					break;
				}
				glTextureStorage2DMultisample(m_handle, sampleCount, glStorageFormat, src.Width(), src.Height(), true);
			}
		}
		if(src.ContainsSourceData())
		{
			SDE_PROF_EVENT("CopyData");
			uint32_t glInternalFormat = SourceFormatToGLInternalFormat(src.SourceFormat());
			uint32_t glInternalType = SourceFormatToGLInternalType(src.SourceFormat());
			SDE_RENDER_ASSERT(glInternalFormat != -1);
			SDE_RENDER_ASSERT(glInternalType != -1);
			if (src.GetAA() == TextureSource::Antialiasing::None)
			{
				for (uint32_t m = 0; m < src.MipCount(); ++m)
				{
					uint32_t w = 0, h = 0;
					size_t size = 0;
					const uint8_t* mipData = src.MipLevel(m, w, h, size);
					SDE_RENDER_ASSERT(mipData);

					glTextureSubImage2D(m_handle, m, 0, 0, w, h, glInternalFormat, glInternalType, mipData);
				}
			}
		}
		if (shouldGenerateMips && src.GetAA() == TextureSource::Antialiasing::None)
		{
			SDE_PROF_EVENT("GenerateMips");
			glGenerateTextureMipmap(m_handle);
		}
		return m_handle != 0;
	}

	bool Texture::CreateSimpleCompressedTexture2D(const TextureSource& src)
	{
		SDE_PROF_EVENT();

		glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);

		const uint32_t mipCount = src.MipCount();
		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

		uint32_t glFormat = SourceFormatToGLStorageFormat(src.SourceFormat());
		SDE_RENDER_ASSERT(glFormat != -1);

		// This preallocates the entire mip-chain
		glTextureStorage2D(m_handle, mipCount, glFormat, src.Width(), src.Height());

		for (uint32_t m = 0; m < mipCount; ++m)
		{
			uint32_t w = 0, h = 0;
			size_t size = 0;
			const uint8_t* mipData = src.MipLevel(m, w, h, size);
			SDE_RENDER_ASSERT(mipData);

			glCompressedTextureSubImage2D(m_handle, m, 0, 0, w, h, glFormat, (GLsizei)size, mipData);
		}
		return m_handle != 0;
	}

	bool Texture::CreateArrayCompressedTexture2D(const std::vector<TextureSource>& src)
	{
		SDE_PROF_EVENT();

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_handle);

		uint32_t glFormat = SourceFormatToGLStorageFormat(src[0].SourceFormat());
		SDE_RENDER_ASSERT(glFormat != -1);

		const uint32_t mipCount = src[0].MipCount();
		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

		// This preallocates the entire mip-chain for all textures in the array
		glTextureStorage3D(m_handle, mipCount, glFormat, src[0].Width(), src[0].Height(), (GLsizei)src.size());

		// Now, since opengl is retarded and requires that glCompressedTexSubImage3D is passed all data for a mip level,
		// we must manually pack all data for a particular mip into one buffer
		uint32_t mip0w, mip0h;
		size_t mip0size;
		src[0].MipLevel(0, mip0w, mip0h, mip0size);	// Use first image to get total size required
		std::vector<uint8_t> mipBuffer(mip0size * src.size());
		for (uint32_t m = 0; m < mipCount; ++m)
		{
			uint32_t w = 0, h = 0;
			size_t size = 0;
			uint8_t* dataPtr = mipBuffer.data();
			for (uint32_t d = 0; d < src.size(); ++d)
			{
				const uint8_t* mipData = src[d].MipLevel(m, w, h, size);
				SDE_RENDER_ASSERT(mipData);
				memcpy(dataPtr, mipData, size);
				dataPtr += size;
			}
			auto bytesWritten = (size_t)(dataPtr - mipBuffer.data());
			SDE_RENDER_ASSERT(bytesWritten <= mipBuffer.size());

			glCompressedTextureSubImage3D(m_handle, m, 0, 0, 0, w, h, (GLsizei)src.size(), glFormat, (GLsizei)(bytesWritten), mipBuffer.data());
		}
		mipBuffer.clear();

		return m_handle != 0;
	}

	bool Texture::ValidateSource(const std::vector<TextureSource>& src)
	{
		if (src.size() == 0)
		{
			return false;
		}
		if (src[0].MipCount() == 0)
		{
			return false;
		}

		uint32_t mipCount = src[0].MipCount();
		uint32_t width = src[0].Width();
		uint32_t height = src[0].Height();
		TextureSource::Format format = src[0].SourceFormat();
		bool has3d = false;
		for (auto& it : src)
		{
			has3d |= it.Is3D();
			if (mipCount != it.MipCount())
			{
				return false;
			}
			if (width != it.Width())
			{
				return false;
			}
			if (height != it.Height())
			{
				return false;
			}
			if (format != it.SourceFormat())
			{
				return false;
			}
		}

		if (has3d)	// dont support 3d texture arrays
		{
			return false;
		}

		return true;
	}

	bool Texture::Update(const std::vector<TextureSource>& src)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle != -1);
		SDE_RENDER_ASSERT(src.size() > 0);
		if (!ValidateSource(src))
		{
			SDE_LOGC("Source data not valid for this texture");
			return false;
		}

		// only support uncompressed flat textures for now, laziness inbound
		if (ShouldCreateCompressed(src[0].SourceFormat()) || src.size() != 1)
		{
			return false;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(src[0].GetDataRowAlignment()));

		uint32_t glStorageFormat = SourceFormatToGLStorageFormat(src[0].SourceFormat());
		uint32_t glInternalFormat = SourceFormatToGLInternalFormat(src[0].SourceFormat());
		uint32_t glInternalType = SourceFormatToGLInternalType(src[0].SourceFormat());
		SDE_RENDER_ASSERT(glStorageFormat != -1);
		SDE_RENDER_ASSERT(glInternalFormat != -1);
		SDE_RENDER_ASSERT(glInternalType != -1);
		const uint32_t mipCount = src[0].MipCount();
		for (uint32_t m = 0; m < mipCount; ++m)
		{
			uint32_t w = 0, h = 0;
			size_t size = 0;
			const uint8_t* mipData = src[0].MipLevel(m, w, h, size);
			SDE_RENDER_ASSERT(mipData);

			glTextureSubImage2D(m_handle, m, 0, 0, w, h, glInternalFormat, glInternalType, mipData);
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		return true;
	}

	bool Texture::MakeResidentHandle()
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle != -1);
		SDE_RENDER_ASSERT(m_residentHandle == 0);
		m_residentHandle = glGetTextureHandleARB(m_handle);
		glMakeTextureHandleResidentARB(m_residentHandle);
		return m_residentHandle != 0;
	}

	bool Texture::CreateCubemap(const TextureSource& src, bool makeResidentNow)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle == -1);

		glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(src.GetDataRowAlignment()));

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_handle);

		m_componentCount = SourceFormatToComponentCount(src.SourceFormat());
		const bool shouldGenerateMips = src.MipCount() <= 1 && src.ShouldGenerateMips();
		const uint32_t mipCount = shouldGenerateMips ? GetGeneratedMipCount(src) : src.MipCount();

		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (mipCount > 1)
		{
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else
		{
			glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glTextureParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		uint32_t glStorageFormat = SourceFormatToGLStorageFormat(src.SourceFormat());
		SDE_RENDER_ASSERT(glStorageFormat != -1);
		{
			SDE_PROF_EVENT("AllocateStorage");
			// This preallocates the entire mip-chain and all faces
			glTextureStorage2D(m_handle, std::max(mipCount, 1u), glStorageFormat, src.Width(), src.Height());
		}

		if (src.ContainsSourceData())
		{
			SDE_PROF_EVENT("CopyData");
			uint32_t glInternalFormat = SourceFormatToGLInternalFormat(src.SourceFormat());
			uint32_t glInternalType = SourceFormatToGLInternalType(src.SourceFormat());
			SDE_RENDER_ASSERT(glInternalFormat != -1);
			SDE_RENDER_ASSERT(glInternalType != -1);
			for (uint32_t face = 0; face < 6; ++face)
			{
				for (uint32_t m = 0; m < src.MipCount(); ++m)
				{
					uint32_t w = 0, h = 0;
					size_t size = 0;
					const uint8_t* mipData = src.MipLevel(m, w, h, size);
					SDE_RENDER_ASSERT(mipData);

					glTextureSubImage3D(m_handle, m, 0, 0, face, w, h, 1, glInternalFormat, glInternalType, mipData);
				}
			}
		}
		if (shouldGenerateMips)
		{
			SDE_PROF_EVENT("GenerateMips");
			glGenerateTextureMipmap(m_handle);
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		if (makeResidentNow)
		{
			return MakeResidentHandle();
		}
		return m_handle != 0;
	}

	bool Texture::Create(const TextureSource& src, bool makeResidentNow)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle == -1);
		m_isArray = false;
		bool createdOk = false;

		glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(src.GetDataRowAlignment()));

		if (src.Is3D())
		{
			assert(!ShouldCreateCompressed(src.SourceFormat()));
			createdOk = CreateSimpleUncompressedTexture3D(src);
		}
		else if (ShouldCreateCompressed(src.SourceFormat()))
		{
			createdOk = CreateSimpleCompressedTexture2D(src);
		}
		else
		{
			createdOk = CreateSimpleUncompressedTexture2D(src);
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		if (makeResidentNow)
		{
			return MakeResidentHandle();
		}
		return createdOk;
	}

	bool Texture::Create(const std::vector<TextureSource>& src, bool makeResidentNow)
	{
		SDE_PROF_EVENT();
		SDE_RENDER_ASSERT(m_handle == -1);
		SDE_RENDER_ASSERT(src.size() > 0);
		if (!ValidateSource(src))
		{
			SDE_LOGC("Source data not valid for this texture");
			return false;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(src[0].GetDataRowAlignment()));

		bool createdOk = false;
		if (ShouldCreateCompressed(src[0].SourceFormat()))
		{
			if (src.size() == 1)
			{
				m_isArray = false;
				createdOk = CreateSimpleCompressedTexture2D(src[0]);
			}
			else
			{
				m_isArray = true;
				createdOk = CreateArrayCompressedTexture2D(src);
			}
		}
		else if (src.size() == 1)
		{
			m_isArray = false;
			createdOk = CreateSimpleUncompressedTexture2D(src[0]);
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		if (makeResidentNow)
		{
			return MakeResidentHandle();
		}
		return createdOk;
	}

	void Texture::Destroy()
	{
		SDE_PROF_EVENT();
		if (m_residentHandle != 0)
		{
			glMakeTextureHandleNonResidentARB(m_residentHandle);
			m_residentHandle = 0;
		}
		if (m_handle != -1)
		{
			glDeleteTextures(1, &m_handle);
			m_handle = -1;
		}
	}
}