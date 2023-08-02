#include "time_system.h"
#include "core/time.h"
#include "core/log.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "script_system.h"

namespace Engine
{
	TimeSystem::TimeSystem()
	{
		m_tickFrequency = Core::Time::HighPerformanceCounterFrequency();
		m_constructTime = Core::Time::HighPerformanceCounterTicks();
		m_fixedUpdateDeltaTime = (double)m_tickFrequency * (1.0 / 60.0);	// 60 fps fixed update

		// Register tick fns
		auto& sm = Engine::SystemManager::GetInstance();
		sm.RegisterTickFn("Time::UpdateGui", [this]() {
			return UpdateGui();
		});
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

	double TimeSystem::GetElapsedTimeReal() const
	{
		return (Core::Time::HighPerformanceCounterTicks() - m_firstTickTime) / double(m_tickFrequency);
	}

	double TimeSystem::GetElapsedTime() const
	{
		return (m_thisFrameTickTime - m_firstTickTime) / double(m_tickFrequency);
	}

	double TimeSystem::GetVariableDeltaTime() const
	{
		return (m_thisFrameTickTime - m_lastTickTime) / double(m_tickFrequency);
	}

	bool TimeSystem::FixedUpdateEnd()
	{
		m_fixedUpdateCatchup -= m_fixedUpdateDeltaTime;
		return true;
	}

	double TimeSystem::GetFixedUpdateCatchupTime() const
	{
		return m_fixedUpdateCatchup / double(m_tickFrequency);
	}

	double TimeSystem::GetFixedUpdateDelta() const
	{
		return m_fixedUpdateDeltaTime / double(m_tickFrequency);
	}

	bool TimeSystem::UpdateGui()
	{
		auto dbgGui = Engine::GetSystem<DebugGuiSystem>("DebugGui");

		Engine::MenuBar mainMenu;
		auto& particlesMenu = mainMenu.AddSubmenu(ICON_FK_CLOCK_O " Time");
		particlesMenu.AddItem("Show GUI", [this]() {
			m_showGui = true;
			});
		dbgGui->MainMenuBar(mainMenu);

		if (m_showGui)
		{
			if (dbgGui->BeginWindow(m_showGui, "Time"))
			{
				dbgGui->Text("Tick Frequency: %ull", m_tickFrequency);
				dbgGui->Text("Total time elapsed: %f", (m_thisFrameTickTime - m_constructTime) / double(m_tickFrequency));
				dbgGui->Text("Variable Update delta: %f", GetVariableDeltaTime());
				dbgGui->Text("Fixed Update delta: %f", GetFixedUpdateDelta());
				dbgGui->Text("Fixed Update catchup time: %f", GetFixedUpdateCatchupTime());
				dbgGui->EndWindow();
			}
		}
		return true;
	}

	bool TimeSystem::Tick(float timeDelta)
	{
		if (m_firstTick)
		{
			m_firstTickTime = Core::Time::HighPerformanceCounterTicks();
			m_thisFrameTickTime = m_firstTickTime;
			m_lastTickTime = m_thisFrameTickTime;
			m_firstTick = false;
			SDE_LOG("Initialise took %f seconds", (m_firstTickTime - m_preInitTime) / double(m_tickFrequency));
		}

		m_thisFrameTickTime = Core::Time::HighPerformanceCounterTicks();
		uint64_t elapsed = m_thisFrameTickTime - m_lastTickTime;
		m_lastTickTime = m_thisFrameTickTime;
		m_fixedUpdateCatchup += elapsed;

		return true;
	}

}