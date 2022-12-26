#pragma once

#include "system.h"

namespace Engine
{
	// Useful time stuff (elapsed time, deltas, etc)
	class TimeSystem : public System
	{
	public:
		TimeSystem();
		bool PreInit();
		bool Initialise();
		bool PostInit();
		bool Tick(float timeDelta);
		double GetElapsedTime() const;	// time since first tick

	private:
		uint64_t m_tickFrequency = 1;
		uint64_t m_constructTime = 1;
		uint64_t m_preInitTime = 1;
		uint64_t m_initTime = 1;
		uint64_t m_postInitTime = 1;
		uint64_t m_firstTickTime = 1;
		uint64_t m_lastTickTime = 1;
		uint64_t m_thisFrameTickTime = 1;
		bool m_firstTick = true;
	};
}