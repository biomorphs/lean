#pragma once
#include "serialisation.h"
#include "system.h"
#include "render/texture.h"
#include "render/texture_source.h"
#include "core/mutex.h"
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>

namespace Engine
{
	class DebugGuiSystem;

	struct TextureHandle
	{
		SERIALISED_CLASS();
		uint32_t m_index = -1u;
		static TextureHandle Invalid() { return { (uint32_t)-1 }; };
	};

	class TextureManager : public System
	{
	public:
		TextureHandle LoadTexture(std::string path, std::function<void(bool, TextureHandle)> onFinish = nullptr);
		Render::Texture* GetTexture(const TextureHandle& h);
		std::string GetTexturePath(const TextureHandle& h);
		void ReloadAll();

		virtual bool Tick(float timeDelta);
		virtual void Shutdown();

	private:
		void ProcessLoadedTextures();
		bool ShowGui(DebugGuiSystem& gui);

		struct TextureDesc {
			std::unique_ptr<Render::Texture> m_texture;
			std::string m_path;
		};
		std::vector<TextureDesc> m_textures;

		struct LoadedTexture
		{
			std::unique_ptr<Render::Texture> m_texture;
			TextureHandle m_destination;
			std::function<void(bool, TextureHandle)> m_onFinish;
		};
		Core::Mutex m_loadedTexturesMutex;
		std::vector<LoadedTexture> m_loadedTextures;
		std::atomic<int32_t> m_inFlightTextures = 0;
	};
}