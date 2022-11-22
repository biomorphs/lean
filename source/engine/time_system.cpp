#include "time_system.h"
#include "core/time.h"
#include "core/log.h"
#include "engine/system_manager.h"
#include "script_system.h"

namespace Engine
{

	TimeSystem::TimeSystem()
	{
		m_tickFrequency = Core::Time::HighPerformanceCounterFrequency();
		m_constructTime = Core::Time::HighPerformanceCounterTicks();
	}

	bool TimeSystem::PreInit()
	{
		m_preInitTime = Core::Time::HighPerformanceCounterTicks();
		return true;
	}

	bool TimeSystem::Initialise()
	{
		m_initTime = Core::Time::HighPerformanceCounterTicks();
		return true;
	}

	bool TimeSystem::PostInit()
	{
		auto scripts = Engine::GetSystem<ScriptSystem>("Script");
		auto time = scripts->Globals()["Time"].get_or_create<sol::table>();
		time["GetElapsedTime"] = [this]() {
			return GetElapsedTime();
		};

		m_postInitTime = Core::Time::HighPerformanceCounterTicks();
		
		return true;
	}

	double TimeSystem::GetElapsedTime() const
	{
		return (Core::Time::HighPerformanceCounterTicks() - m_firstTickTime) / double(m_tickFrequency);
	}

	bool TimeSystem::Tick(float timeDelta)
	{
		if (m_firstTick)
		{
			m_firstTickTime = Core::Time::HighPerformanceCounterTicks();
			m_thisFrameTickTime = m_firstTickTime;
			m_firstTick = false;
			SDE_LOG("Initialise took %f seconds", (m_firstTickTime - m_preInitTime) / double(m_tickFrequency));
		}
		m_lastTickTime = m_thisFrameTickTime;
		m_thisFrameTickTime = Core::Time::HighPerformanceCounterTicks();
		return true;
	}

}