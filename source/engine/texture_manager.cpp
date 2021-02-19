#include "texture_manager.h"
#include "job_system.h"
#include "debug_gui_system.h"
#include "core/profiler.h"
#include "core/thread.h"
#include "render/device.h"
#include "stb_image.h"
#include <cassert>

namespace Engine
{
	TextureManager::TextureManager(Engine::JobSystem* js)
		: m_jobSystem(js)
	{
	}

	bool TextureManager::ShowGui(DebugGuiSystem& gui)
	{
		static bool s_showWindow = true;
		static TextureHandle s_showTexture;
		gui.BeginWindow(s_showWindow, "TextureManager");
		char text[1024] = { '\0' };
		int32_t inFlight = m_inFlightTextures;
		sprintf_s(text, "Loading: %d", inFlight);
		gui.Text(text);
		gui.Separator();
		for (int t=0;t<m_textures.size();++t)
		{
			sprintf_s(text, "%d: %s (0x%p) - %d components", 
				t, 
				m_textures[t].m_path.c_str(),
				m_textures[t].m_texture.get(),
				m_textures[t].m_texture ? m_textures[t].m_texture->GetComponentCount() : 0);
			if (gui.Button(text))
			{
				s_showTexture = { static_cast<uint16_t>(t) };
			}
		}
		gui.EndWindow();

		if(s_showTexture.m_index != -1)
		{ 
			auto previewTexture = GetTexture(s_showTexture);
			if (previewTexture != nullptr)
			{
				bool show = true;
				gui.BeginWindow(show, m_textures[s_showTexture.m_index].m_path.c_str());
				gui.Image(*previewTexture, glm::vec2(512, 512));
				gui.EndWindow();
				if (!show)
				{
					s_showTexture = {(uint16_t)-1};
				}
			}
		}

		return s_showWindow;
	}

	void TextureManager::ReloadAll()
	{
		SDE_PROF_EVENT();

		// wait until all jobs finish, not great but eh
		while (m_inFlightTextures > 0)
		{
			int v = m_inFlightTextures;
			Core::Thread::Sleep(1);
		}
		// clear out the old results
		{
			Core::ScopedMutex guard(m_loadedTexturesMutex);
			m_loadedTextures.clear();
		}
		// now load the textures again
		auto currentTextures = std::move(m_textures);
		for (int t=0;t<currentTextures.size();++t)
		{
			auto newHandle = LoadTexture(currentTextures[t].m_path.c_str());
			assert(t == newHandle.m_index);
		}
	}

	void TextureManager::ProcessLoadedTextures()
	{
		SDE_PROF_EVENT();

		std::vector<LoadedTexture> loadedTextures;
		{
			SDE_PROF_EVENT("AcquireLoadedTextures");
			Core::ScopedMutex guard(m_loadedTexturesMutex);
			loadedTextures = std::move(m_loadedTextures);
		}

		{
			SDE_PROF_EVENT("CreateTextures");
			for (auto& tex : loadedTextures)
			{
				if(tex.m_texture != nullptr)
				{
					m_textures[tex.m_destination.m_index].m_texture = std::move(tex.m_texture);
				}
			}
		}
	}

	TextureHandle TextureManager::LoadTexture(std::string path)
	{
		if (path.empty())
		{
			return TextureHandle::Invalid();
		}

		for (uint64_t i = 0; i < m_textures.size(); ++i)
		{
			if (m_textures[i].m_path == path)
			{
				return { static_cast<uint16_t>(i) };
			}
		}

		m_textures.push_back({nullptr, path });
		auto newHandle = TextureHandle{ static_cast<uint16_t>(m_textures.size() - 1) };
		m_inFlightTextures += 1;

		std::string pathString = path;
		m_jobSystem->PushJob([this, pathString, newHandle](void*) {
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "LoadTexture %s", pathString.c_str());
			SDE_PROF_EVENT_DYN(debugName);

			int w, h, components;
			stbi_set_flip_vertically_on_load(true);
			unsigned char* loadedData = stbi_load(pathString.c_str(), &w, &h, &components, 0);
			if (loadedData == nullptr)
			{
				m_inFlightTextures -= 1;
				return;
			}

			std::vector<uint8_t> rawDataBuffer;
			rawDataBuffer.resize(w * h * components);
			memcpy(rawDataBuffer.data(), loadedData, w * h * components);
			stbi_image_free(loadedData);

			std::vector<Render::TextureSource::MipDesc> mip;
			mip.push_back({ (uint32_t)w,(uint32_t)h,0,w * h * (size_t)components });
			Render::TextureSource::Format format = Render::TextureSource::Format::Unsupported;
			switch (components)
			{
			case 1:
				format = Render::TextureSource::Format::R8;
				break;
			case 3:
				format = Render::TextureSource::Format::RGB8;
				break;
			case 4:
				format = Render::TextureSource::Format::RGBA8;
				break;
			}
			Render::TextureSource ts((uint32_t)w, (uint32_t)h, format, { mip }, rawDataBuffer);
			ts.SetGenerateMips(true);
			auto newTex = std::make_unique<Render::Texture>();
			if (newTex->Create(ts))
			{
				// Ensure any writes are shared with all contexts
				Render::Device::FlushContext();

				SDE_PROF_EVENT("PushToResultsList");
				Core::ScopedMutex guard(m_loadedTexturesMutex);
				{
					m_loadedTextures.push_back({ std::move(newTex), newHandle });
				}
			}
			m_inFlightTextures -= 1;
		});

		return newHandle;
	}

	Render::Texture* TextureManager::GetTexture(const TextureHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_textures.size())
		{
			return m_textures[h.m_index].m_texture.get();
		}
		else
		{
			return nullptr;
		}
	}

}