/*
SDLEngine
Matt Hoyle
*/
#include "event_system.h"
#include <algorithm>
#include <SDL_events.h>
#include "core/profiler.h"

namespace Engine
{
	EventSystem::EventSystem()
	{

	}

	EventSystem::~EventSystem()
	{

	}

	void EventSystem::RegisterEventHandler(EventHandler h)
	{
		m_handlers.push_back(h);
	}

	bool EventSystem::Tick()
	{
		SDE_PROF_EVENT("EventSystem::Tick");
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			SDE_PROF_EVENT("HandleEvent");
			for (auto& it : m_handlers)
			{
				it(&event);
			}
			if (event.type == SDL_QUIT)
			{
				return false;
			}
		}
		return true;
	}
}