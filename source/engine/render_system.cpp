#include "render_system.h"
#include "job_system.h"
#include "engine/system_manager.h"
#include "render/window.h"
#include "render/device.h"
#include "core/profiler.h"
#include "core/log.h"
#include <algorithm>

namespace Engine
{
	RenderSystem::DevicePresenter::DevicePresenter(RenderSystem* r)
		: m_renderSystem(r)
	{
	}

	bool RenderSystem::DevicePresenter::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		if (m_renderSystem->m_device != nullptr)
		{
			m_renderSystem->m_device->Present();
		}

		return true;
	}

	RenderSystem::RenderSystem()
	{
	}

	RenderSystem::~RenderSystem()
	{
	}

	RenderSystem::DevicePresenter* RenderSystem::MakePresenter()
	{
		return new DevicePresenter(this);
	}

	bool RenderSystem::PreInit()
	{
		SDE_PROF_EVENT();
		m_jobSystem = (JobSystem*)Engine::GetSystem("Jobs");

		// Create the window + device asap
		Render::Window::Properties winProps(m_config.m_windowTitle, m_config.m_windowWidth, m_config.m_windowHeight);
		winProps.m_flags = m_config.m_fullscreen ? Render::Window::CreateFullscreen : 0;
		m_window = std::make_unique<Render::Window>(winProps);
		if (!m_window)
		{
			SDE_LOGC(SDE, "Failed to create window");
			return false;
		}

		m_device = std::make_unique<Render::Device>(*m_window, m_config.m_debugContext);
		if (!m_device)
		{
			SDE_LOGC(SDE, "Failed to create render device");
			return false;
		}

		return true;
	}

	bool RenderSystem::Initialise()
	{
		SDE_PROF_EVENT();

		// We need to create a opengl context for each job thread before the job system starts in PostInit
		auto threadCount = m_jobSystem->GetThreadCount();
		for (int i = 0; i < threadCount; ++i)
		{
			void* sharedContext = m_device->CreateSharedGLContext();
			if (sharedContext == nullptr)
			{
				SDE_LOG("Error when creating shared GL context. Too many threads?");
				return false;
			}
			m_renderContexts.push_back(sharedContext);
		}
		// Creating a context sets it by default, so make sure we reset the main thread context
		m_device->SetGLContext(m_device->GetGLContext());

		// Finally tell the job system threads about our contexts
		auto jobThreadInit = [this](uint32_t threadIndex)
		{
			m_device->SetGLContext(m_renderContexts[threadIndex]);
		};
		m_jobSystem->SetThreadInitFn(jobThreadInit);

		return true;
	}

	bool RenderSystem::PostInit()
	{
		SDE_PROF_EVENT();
		m_window->Show();
		return true;
	}

	bool RenderSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		// bind backbuffer for drawing
		m_device->DrawToBackbuffer();
		m_device->SetViewport({ 0,0 }, { m_config.m_windowWidth, m_config.m_windowHeight });

		for (auto renderPass : m_passes)
		{
			renderPass.m_pass->RenderAll(*m_device);
			renderPass.m_pass->Reset();
		}

		return true;
	}

	void RenderSystem::PostShutdown()
	{
		SDE_PROF_EVENT();

		m_passes.clear();
		m_device = nullptr;
		m_window = nullptr;
	}

	void RenderSystem::AddPass(Render::RenderPass& pass, uint32_t sortKey)
	{
		if (sortKey == -1)
		{
			sortKey = m_lastSortKey++;
		}
		m_passes.push_back({ &pass, sortKey });
		std::sort(m_passes.begin(), m_passes.end(), [](const Pass& a, Pass& b) {
			return a.m_key < b.m_key;
		});
	}
}