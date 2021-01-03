#include "platform.h"
#include "core/log.h"
#include <cassert>
#include <SDL.h>

namespace Platform
{
	int CPUCount()
	{
		return SDL_GetCPUCount();
	}

	InitResult Platform::Initialise(int argc, char* argv[])
	{
		SDE_LOGC(Engine, "Initialising Platform");

		int sdlResult = SDL_Init(SDL_INIT_EVERYTHING);
		assert(sdlResult == 0);

		if (sdlResult != 0)
		{
			SDE_LOGC(Engine, "Failed to initialise SDL:\r\n\t%s", SDL_GetError());
			return InitResult::InitFailed;
		}

		return InitResult::InitOK;
	}

	ShutdownResult Platform::Shutdown()
	{
		SDE_LOGC(Engine, "Shutting down Platform");

		SDL_Quit();

		return ShutdownResult::ShutdownOK;
	}
}
