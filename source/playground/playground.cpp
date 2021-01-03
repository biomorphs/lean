#include "playground.h"
#include "core/file_io.h"
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

void Playground::NewScene()
{

}

void Playground::ReloadScripts()
{
	std::string scriptErrors;
	m_loadedSceneScripts.clear();
	for (auto it : m_scene.Scripts())
	{
		sol::table scriptTable;
		m_scriptSystem->RunScriptFromFile(it.c_str(), scriptErrors, scriptTable);
		if (scriptTable.valid() && scriptTable["Init"] != nullptr)
		{
			try
			{
				scriptTable["Init"]();
			}
			catch (const sol::error& err)
			{
				SDE_LOG("Lua Error - %s", err.what());
			}
		}
		m_loadedSceneScripts.push_back(std::move(scriptTable));
	}
	if (scriptErrors.length() > 0)
	{
		SDE_LOG("Script failure: %s", scriptErrors.c_str());
	}
}

void Playground::LoadScene(std::string filename)
{
	std::string fileContents;
	m_scene = {};
	if (Core::LoadTextFromFile(filename, fileContents))
	{
		nlohmann::json json = nlohmann::json::parse(fileContents);
		m_scene.Serialise(json, Engine::Serialiser::Reader);

		ReloadScripts();
	}
}

void Playground::SaveScene(std::string filename)
{
	nlohmann::json json;
	m_scene.Serialise(json, Engine::Serialiser::Writer);
	Core::SaveTextToFile(filename, json.dump(2));
}

void Playground::TickScene()
{
	for (int script=0;script< m_scene.Scripts().size();++script)
	{
		sol::table& cachedTable = m_loadedSceneScripts[script];
		if (cachedTable.valid() && cachedTable["Tick"] != nullptr)
		{
			try
			{
				cachedTable["Tick"](m_deltaTime);
			}
			catch (const sol::error& err)
			{
				SDE_LOG("Lua Error - %s", err.what());
			}
		}
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

	Scene testScene;
	testScene.Name() = "Playground Test Scene";
	testScene.Scripts().push_back("playground.lua");
	m_sceneFilename = "playground.scene";
	nlohmann::json json;
	testScene.Serialise(json, Engine::Serialiser::Writer);
	Core::SaveTextToFile(m_sceneFilename, json.dump(2));

	m_sceneEditor.Init(&m_scene, m_debugGui);

	auto& fileMenu = g_menuBar.AddSubmenu(ICON_FK_FILE_O " File");
	fileMenu.AddItem("Exit", []() { g_keepRunning = false; });

	auto& scriptMenu = g_menuBar.AddSubmenu(ICON_FK_GLOBE " Scene");
	scriptMenu.AddItem("Load Scene", [this] { LoadScene(m_sceneFilename); });
	scriptMenu.AddItem("Toggle Editor", [this] { m_sceneEditor.ToggleEnabled(); });
	scriptMenu.AddItem("Toggle Paused", [] { g_pauseScriptDelta = !g_pauseScriptDelta; });

	return true;
}

bool Playground::Tick()
{
	SDE_PROF_EVENT();

	m_debugGui->MainMenuBar(g_menuBar);
	if (m_sceneEditor.Tick())
	{
		ReloadScripts();
	}

	double thisFrameTime = m_timer.GetSeconds();
	if (!g_pauseScriptDelta)
	{
		m_deltaTime = thisFrameTime - m_lastFrameTime;
	}
	else
	{
		m_deltaTime = 0.0f;
	}
	TickScene();
	m_lastFrameTime = thisFrameTime;

	return g_keepRunning;
}

void Playground::Shutdown()
{
	SDE_PROF_EVENT();
	m_loadedSceneScripts.clear();
}