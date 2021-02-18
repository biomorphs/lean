#pragma once

#include <stdint.h>

namespace Core
{
	namespace Time
	{
		uint64_t HighPerformanceCounterTicks();
		uint64_t HighPerformanceCounterFrequency();
	}
}