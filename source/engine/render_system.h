#pragma once

#include "system.h"
#include "core/glm_headers.h"
#include "render/render_pass.h"
#include <vector>
#include <memory>
#include <string>

namespace Render
{
	class Device;
	class Window;
	class Camera;
	class RenderPass;
}

namespace Engine
{
	class JobSystem;

	// Main renderer owns device and submits passes to gpu
	class RenderSystem : public System
	{
	public:
		RenderSystem();
		virtual ~RenderSystem();

		void AddPass(Render::RenderPass& pass, uint32_t sortKey = -1);

		inline Render::Window* GetWindow() { return m_window.get(); }
		inline Render::Device* GetDevice() { return m_device.get(); }

		bool PreInit(SystemEnumerator& systemEnumerator);
		bool Initialise();		// Window and device are created here
		bool PostInit();		// Window made visible
		bool Tick(float timeDelta);			// All passes are drawn here
		void PostShutdown();	// Device + window shutdown here

	private:
		struct Config
		{
			uint32_t m_windowWidth = 1920;
			uint32_t m_windowHeight = 1080;
			std::string m_windowTitle = "LEAN";
			bool m_fullscreen = false;
		} m_config;

		struct Pass { Render::RenderPass* m_pass; uint32_t m_key; };
		std::vector<Pass> m_passes;
		uint32_t m_lastSortKey = 0;

		std::unique_ptr<Render::Window> m_window;
		std::unique_ptr<Render::Device> m_device;
		std::vector<void*> m_renderContexts;	// per-thread contexts
		JobSystem* m_jobSystem = nullptr;
	};
}