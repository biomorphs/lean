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
		bool Tick(float timeDelta);					// variable update
		bool FixedUpdateEnd();						// called at end of each fixed update

		double GetElapsedTimeReal() const;			// return the actual elapsed time

		double GetElapsedTime() const;				// total time from first tick to current variable tick
		double GetVariableDeltaTime() const;		// variable time delta from previous frame

		double GetFixedUpdateCatchupTime() const;	// how much time does fixed update need to catch up
		double GetFixedUpdateDelta() const;			// fixed update delta time

	private:
		uint64_t m_tickFrequency = 1;
		uint64_t m_constructTime = 1;
		uint64_t m_preInitTime = 1;
		uint64_t m_initTime = 1;
		uint64_t m_postInitTime = 1;
		uint64_t m_firstTickTime = 1;
		uint64_t m_lastTickTime = 1;
		uint64_t m_thisFrameTickTime = 1;
		uint64_t m_fixedUpdateCatchup = 0;
		uint64_t m_fixedUpdateDeltaTime = 1;
		uint64_t m_fixedUpdateCurrentTime = 1;
		bool m_firstTick = true;
		bool m_showGui = false;
		bool UpdateGui();
	};
}