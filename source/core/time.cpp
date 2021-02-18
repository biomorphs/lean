#include "time.h"
#include <sdl_timer.h>

namespace Core
{
	namespace Time
	{
		uint64_t HighPerformanceCounterTicks()
		{
			return SDL_GetPerformanceCounter();
		}

		uint64_t HighPerformanceCounterFrequency()
		{
			return SDL_GetPerformanceFrequency();
		}
	}
}