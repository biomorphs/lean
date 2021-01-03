#pragma once

#include <stdint.h>

namespace Kernel
{
	namespace Time
	{
		uint64_t HighPerformanceCounterTicks();
		uint64_t HighPerformanceCounterFrequency();
	}
}