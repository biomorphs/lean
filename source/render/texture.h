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
			m_residentHandle = other.m_residentHandle;
			other.m_handle = -1;
			other.m_residentHandle = 0;
		}
		~Texture();

		bool Update(const std::vector<TextureSource>& src);
		bool Create(const std::vector<TextureSource>& src, bool makeResidentNow=true);
		bool CreateCubemap(const TextureSource& src, bool makeResidentNow = true);
		bool Create(const TextureSource& src, bool makeResidentNow = true);
		void Destroy();
		bool MakeResidentHandle();	// must be called from main render context!

		inline uint64_t GetResidentHandle() const { return m_residentHandle; }	// bindless texture handle for shaders
		inline uint32_t GetHandle() const { return m_handle; }	// gl texture object
		inline bool IsArray() const { return m_isArray; }
		uint32_t GetComponentCount() const { return m_componentCount; }

		void SetClampToBorder(glm::vec4 borderColour);

	private:
		bool CreateSimpleUncompressedTexture3D(const TextureSource& src);
		bool CreateSimpleUncompressedTexture2D(const TextureSource& src);
		bool CreateSimpleCompressedTexture2D(const TextureSource& src);
		bool CreateArrayCompressedTexture2D(const std::vector<TextureSource>& src);
		bool ValidateSource(const std::vector<TextureSource>& src);

		bool m_isArray = false;
		uint32_t m_handle = -1;		// cpu-side handle
		uint32_t m_componentCount = 0;
		uint64_t m_residentHandle = 0;	// gpu-side handle (bindless!)
	};
}