#include "playground.h"
#include "core/time.h"
#include "core/log.h"
#include "core/profiler.h"
#include "engine/system_enumerator.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"
#include "debug_gui_script_binding.h"

Playground::Playground()
{
	srand((uint32_t)Kernel::Time::HighPerformanceCounterTicks());
}

Playground::~Playground()
{
}

void Playground::CallScriptInit()
{
	SDE_PROF_EVENT();
	try
	{
		if(m_scriptFunctions.valid() && m_scriptFunctions["Init"] != nullptr)
		{
			sol::function initFn = m_scriptFunctions["Init"];
			initFn();
		}
	}
	catch (const sol::error& err)
	{
		SDE_LOG("Lua Error - %s", err.what());
		CallScriptShutdown();
	}
}

void Playground::CallScriptTick()
{
	SDE_PROF_EVENT();
	try
	{
		if (m_scriptFunctions.valid() && m_scriptFunctions["Tick"] != nullptr)
		{
			sol::function tickFn = m_scriptFunctions["Tick"];
			tickFn(m_deltaTime);
		}
	}
	catch (const sol::error& err)
	{
		SDE_LOG("Lua Error - %s", err.what());
		CallScriptShutdown();
	}
}

void Playground::CallScriptShutdown()
{
	SDE_PROF_EVENT();
	try
	{
		if (m_scriptFunctions.valid() && m_scriptFunctions["Shutdown"] != nullptr)
		{
			sol::function shutdownFn = m_scriptFunctions["Shutdown"];
			shutdownFn();
		}
	}
	catch (const sol::error& err)
	{
		SDE_LOG("Lua Error - %s", err.what());
	}
}

void Playground::ReloadScript()
{
	SDE_PROF_EVENT();
	CallScriptShutdown();
	m_scriptErrorText = "";
	sol::table resultTable;
	if (m_scriptSystem->RunScriptFromFile(m_scriptPath.c_str(), m_scriptErrorText, resultTable))
	{
		m_scriptFunctions = resultTable;
		CallScriptInit();
	}
	else
	{
		SDE_LOG("Lua Error - %s", m_scriptErrorText.c_str());
	}
}

Engine::MenuBar g_menuBar;
bool g_keepRunning = true;
bool g_pauseScriptDelta = false;

bool Playground::PreInit(Engine::SystemEnumerator& systemEnumerator)
{
	SDE_PROF_EVENT();
	m_debugGui = (Engine::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_scriptSystem = (Engine::ScriptSystem*)systemEnumerator.GetSystem("Script");
	m_lastFrameTime = m_timer.GetSeconds();

	DebugGuiScriptBinding::Go(m_debugGui, m_scriptSystem->Globals());

	auto& fileMenu = g_menuBar.AddSubmenu(ICON_FK_FILE_O " File");
	fileMenu.AddItem("Exit", []() { g_keepRunning = false; });

	auto& scriptMenu = g_menuBar.AddSubmenu(ICON_FK_COG " Scripts");
	scriptMenu.AddItem("Reload", [this]() {ReloadScript(); }, "R");
	scriptMenu.AddItem("Toggle Paused", [] { g_pauseScriptDelta = !g_pauseScriptDelta; });

	return true;
}

bool Playground::Tick()
{
	SDE_PROF_EVENT();
	static bool s_firstTick = true;
	if (s_firstTick)
	{
		s_firstTick = false;
		ReloadScript();
	}

	m_debugGui->MainMenuBar(g_menuBar);

	double thisFrameTime = m_timer.GetSeconds();
	if (!g_pauseScriptDelta)
	{
		m_deltaTime = thisFrameTime - m_lastFrameTime;
	}
	else
	{
		m_deltaTime = 0.0f;
	}
	CallScriptTick();
	m_lastFrameTime = thisFrameTime;

	return g_keepRunning;
}

void Playground::Shutdown()
{
	SDE_PROF_EVENT();
	CallScriptShutdown();
}