#pragma once
#include "core/glm_headers.h"
#include <stdint.h>
#include <vector>

namespace Render
{
	class TextureSource;
	class Texture
	{
	public:
		Texture();
		Texture(const Texture&) = delete;
		Texture(Texture&& other) {
			m_isArray = other.m_isArray;
			m_handle = other.m_handle;
			m_componentCount = other.m_componentCount;
			other.m_handle = -1;
		}
		~Texture();

		bool Update(const std::vector<TextureSource>& src);
		bool Create(const std::vector<TextureSource>& src);
		bool CreateCubemap(const TextureSource& src);
		bool Create(const TextureSource& src);
		void Destroy();

		inline uint32_t GetHandle() const { return m_handle; }
		inline bool IsArray() const { return m_isArray; }
		uint32_t GetComponentCount() const { return m_componentCount; }

		void SetClampToBorder(glm::vec4 borderColour);

	private:
		bool CreateSimpleUncompressedTexture3D(const TextureSource& src);
		bool CreateSimpleUncompressedTexture2D(const TextureSource& src);
		bool CreateSimpleCompressedTexture2D(const TextureSource& src);
		bool CreateArrayCompressedTexture2D(const std::vector<TextureSource>& src);
		bool ValidateSource(const std::vector<TextureSource>& src);

		bool m_isArray;
		uint32_t m_handle;
		uint32_t m_componentCount;
	};
}