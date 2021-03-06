#pragma once
#include <stdint.h>

namespace Core
{
	// Wraps high-performance timer
	class Timer
	{
	public:
		Timer();
		~Timer();

		uint64_t GetTicks() const;
		double GetSeconds() const;
		inline uint64_t GetFrequency() const	{ return m_frequency; }

	private:
		uint64_t m_frequency;
	};

	class ScopedTimer
	{
	public:
		ScopedTimer(double& resultSeconds);
		~ScopedTimer();
	private:
		Timer m_timer;
		uint64_t m_startTicks;
		double& m_result;
	};
}