#pragma once

#include <stdint.h>
#include <vector>

namespace Render
{
	class TextureSource
	{
	public:
		enum class Format
		{
			DXT1,
			DXT3,
			DXT5,
			R8,
			RGB8,
			RGBA8,
			RGBAF16,
			Depth24Stencil8,
			Depth32,
			Unsupported
		};
		enum class WrapMode
		{
			Repeat,
			ClampToEdge
		};
		enum class Antialiasing		// MSAA textures cannot be used directly in shaders, they must be resolved to a non-MSAA target
		{
			None,
			MSAAx2,
			MSAAx4
		};
		struct MipDesc
		{
			uint32_t m_width;
			uint32_t m_height;
			uintptr_t m_offset;
			size_t m_size;
		};

		TextureSource();
		TextureSource(uint32_t w, uint32_t h, Format f);	// empty texture with no mips
		TextureSource(uint32_t w, uint32_t h, Format f, std::vector<MipDesc>& mips, std::vector<uint8_t>& data);
		TextureSource(uint32_t w, uint32_t h, Format f, std::vector<MipDesc>& mips, std::vector<uint32_t>& data);
		~TextureSource();
		TextureSource(const TextureSource&) = delete;
		TextureSource(TextureSource&&) = default;

		inline Antialiasing GetAA() const { return m_antiAliasing; }
		void SetAA(Antialiasing m) { m_antiAliasing = m; }
		inline WrapMode GetWrapModeS() const { return m_wrapModeS; }
		inline WrapMode GetWrapModeT() const { return m_wrapModeT; }
		void SetWrapMode(WrapMode s, WrapMode t) { m_wrapModeS = s; m_wrapModeT = t; }
		inline uint32_t Width() const { return m_width; }
		inline uint32_t Height() const { return m_height; }
		inline uint32_t MipCount() const { return static_cast<uint32_t>(m_mipDescriptors.size()); }
		inline Format SourceFormat() const { return m_format; }
		const uint8_t* MipLevel(uint32_t mip, uint32_t& w, uint32_t& h, size_t& size) const;
		inline void SetGenerateMips(bool g) { m_generateMips = g; }
		inline bool ShouldGenerateMips() const { return m_generateMips; }
		inline bool ContainsSourceData() const { return m_rawBuffer.size() > 0; }
		inline bool& UseNearestFiltering() { return m_useNearestFiltering; }
		inline const bool& UseNearestFiltering() const { return m_useNearestFiltering; }

	private:
		Antialiasing m_antiAliasing = Antialiasing::None;
		WrapMode m_wrapModeS = WrapMode::Repeat;
		WrapMode m_wrapModeT = WrapMode::Repeat;
		Format m_format;
		uint32_t m_width;
		uint32_t m_height;
		std::vector<MipDesc> m_mipDescriptors;
		std::vector<uint8_t> m_rawBuffer;
		bool m_generateMips = false;
		bool m_useNearestFiltering = false;
	};
}

#include "texture_source.inl"