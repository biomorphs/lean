#pragma once

#include "core/glm_headers.h"
#include <string>

struct SDL_Window;

// This is a wrapper for a render window. A device must be paired with the window
namespace Render
{
	class Window
	{
	public:
		enum CreationFlags
		{
			CreateFullscreen = (1 << 1),
			CreateFullscreenDesktop = (1 << 2),
			CreateResizable = (1 << 3),
			CreateMinimized = (1 << 4),
			CreateMaximized = (1 << 5)
		};
		class Properties
		{
		public:
			Properties(const std::string& title, int sizeX = - 1, int sizeY = -1, int flags = 0)
				: m_title( title )
				, m_sizeX( sizeX )
				, m_sizeY( sizeY )
				, m_flags( flags )
			{

			}
			std::string m_title;
			int m_sizeX;
			int m_sizeY;
			int m_flags;
		};

		Window(const Properties& properties);
		~Window();

		void Show();
		void Hide();
		glm::ivec2 GetSize() const { return { m_properties.m_sizeX, m_properties .m_sizeY}; }

		SDL_Window* GetWindowHandle();
		inline const Properties& GetProperties() { return m_properties; }

	private:
		SDL_Window* m_windowHandle;
		Properties m_properties;
	};
}