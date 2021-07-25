#include "window.h"
#include "utils.h"
#include <SDL.h>

namespace Render
{
	Window::Window(const Properties& props)
		: m_properties(props)
	{
		int windowPosX = SDL_WINDOWPOS_UNDEFINED;
		int windowPosY = SDL_WINDOWPOS_UNDEFINED;
		int windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
		
		if (props.m_flags & CreateFullscreen)
			windowFlags |= SDL_WINDOW_FULLSCREEN;
		else if (props.m_flags & CreateFullscreenDesktop)
			windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		else
		{
			if (props.m_flags & CreateResizable)
				windowFlags |= SDL_WINDOW_RESIZABLE;

			if (props.m_flags & CreateMinimized)
				windowFlags |= SDL_WINDOW_MINIMIZED;
			else if (props.m_flags & CreateResizable)
				windowFlags |= SDL_WINDOW_MAXIMIZED;
		}

		// properties to set before window creation
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		// Request opengl 4.5 context.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		// We want all contexts created by us to share resources
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

		// Enable debug context (slower, but better error messages)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

		// Vsync
		SDL_GL_SetSwapInterval(1);

		//24 bit depth
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		m_windowHandle = SDL_CreateWindow(props.m_title.c_str(), windowPosX, windowPosY, props.m_sizeX, props.m_sizeY, windowFlags);
		SDE_RENDER_ASSERT(m_windowHandle);
	}

	Window::~Window()
	{
		SDL_DestroyWindow(m_windowHandle);
		m_windowHandle = nullptr;
	}

	void Window::Show()
	{
		SDE_RENDER_ASSERT(m_windowHandle);
		SDL_ShowWindow(m_windowHandle);
	}

	void Window::Hide()
	{
		SDE_RENDER_ASSERT(m_windowHandle);
		SDL_HideWindow(m_windowHandle);
	}

	SDL_Window* Window::GetWindowHandle() 
	{
		return m_windowHandle;
	}
}