#pragma once

#include "render/render_pass.h"

namespace Render
{
	class Window;
	class Device;
}

namespace Engine
{
	// uses the standard imgui renderer
	class ImguiSdlGL3RenderPass : public Render::RenderPass
	{
	public:
		ImguiSdlGL3RenderPass(Render::Window* window, Render::Device* device);
		virtual ~ImguiSdlGL3RenderPass();
		void NewFrame();
		void HandleEvent(void*);
		virtual void Reset();
		virtual void RenderAll(class Render::Device&);
	private:
		Render::Window* m_window;
	};
}