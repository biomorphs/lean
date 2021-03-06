#pragma once
#include "render/texture.h"
#include "render/texture_source.h"
#include "core/mutex.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace Engine
{
	class JobSystem;
	class DebugGuiSystem;

	struct TextureHandle
	{
		uint32_t m_index = -1;
		static TextureHandle Invalid() { return { (uint32_t)-1 }; };
	};

	class TextureManager
	{
	public:
		TextureManager(JobSystem* js);
		TextureManager(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = delete;
		~TextureManager() = default;

		TextureHandle LoadTexture(std::string path);
		Render::Texture* GetTexture(const TextureHandle& h);
		std::string GetTexturePath(const TextureHandle& h);
		void ProcessLoadedTextures();

		bool ShowGui(DebugGuiSystem& gui);

		void ReloadAll();

	private:
		struct TextureDesc {
			std::unique_ptr<Render::Texture> m_texture;
			std::string m_path;
		};
		std::vector<TextureDesc> m_textures;

		struct LoadedTexture
		{
			std::unique_ptr<Render::Texture> m_texture;
			TextureHandle m_destination;
		};
		Core::Mutex m_loadedTexturesMutex;
		std::vector<LoadedTexture> m_loadedTextures;
		std::atomic<int32_t> m_inFlightTextures = 0;
		JobSystem* m_jobSystem = nullptr;
	};
}