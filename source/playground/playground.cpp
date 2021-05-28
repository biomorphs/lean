#include "playground.h"
#include "core/file_io.h"
#include "core/time.h"
#include "core/log.h"
#include "core/profiler.h"
#include "core/glm_headers.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"
#include "engine/file_picker_dialog.h"
#include "engine/serialisation.h"
#include "entity/entity_system.h"
#include "creatures/creature_system.h"
#include <cassert>

std::string g_configFile = "playground_config.json";
struct PlaygroundConfig
{
	std::string m_lastLoadedScene = "";
	SERIALISED_CLASS();
} g_playgroundConfig;
SERIALISE_BEGIN(PlaygroundConfig)
SERIALISE_PROPERTY("LastLoadedScene", m_lastLoadedScene);
SERIALISE_END()

Playground::Playground()
{
	srand((uint32_t)Core::Time::HighPerformanceCounterTicks());
}

Playground::~Playground()
{
}

void LoadConfig()
{
	SDE_PROF_EVENT();
	std::string configText;
	if (Core::LoadTextFromFile(g_configFile, configText))
	{
		nlohmann::json json = nlohmann::json::parse(configText);
		g_playgroundConfig = {};
		g_playgroundConfig.Serialise(json, Engine::SerialiseType::Read);
	}
}

void SaveConfig()
{
	SDE_PROF_EVENT();
	nlohmann::json json;
	g_playgroundConfig.Serialise(json, Engine::SerialiseType::Write);
	Core::SaveTextToFile(g_configFile, json.dump(2));
}

void Playground::NewScene()
{
	SDE_PROF_EVENT();
	m_scene = {};
	m_creatures->Reset();
	m_entitySystem->NewWorld();
	g_playgroundConfig.m_lastLoadedScene = "";
	SaveConfig();
}

void Playground::ReloadScripts()
{
	SDE_PROF_EVENT();
	std::string scriptErrors;
	m_loadedSceneScripts.clear();
	m_entitySystem->NewWorld();
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
	SDE_PROF_EVENT();
	std::string fileContents;
	m_scene = {};
	m_entitySystem->NewWorld();
	if (Core::LoadTextFromFile(filename, fileContents))
	{
		nlohmann::json json = nlohmann::json::parse(fileContents);
		m_scene.Serialise(json, Engine::SerialiseType::Read);

		ReloadScripts();
		
		g_playgroundConfig.m_lastLoadedScene = filename;
		SaveConfig();
	}
}

void Playground::SaveScene(std::string filename)
{
	SDE_PROF_EVENT();
	nlohmann::json json;
	m_scene.Serialise(json, Engine::SerialiseType::Write);
	Core::SaveTextToFile(filename, json.dump(2));

	g_playgroundConfig.m_lastLoadedScene = filename;
	SaveConfig();
}

void Playground::TickScene(float timeDelta)
{
	SDE_PROF_EVENT();
	for (int script=0;script< m_scene.Scripts().size();++script)
	{
		sol::table& cachedTable = m_loadedSceneScripts[script];
		if (cachedTable.valid() && cachedTable["Tick"] != nullptr)
		{
			try
			{
				cachedTable["Tick"](timeDelta);
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

bool Playground::PreInit(Engine::SystemManager& manager)
{
	SDE_PROF_EVENT();
	m_systemManager = &manager;
	m_debugGui = (Engine::DebugGuiSystem*)manager.GetSystem("DebugGui");
	m_scriptSystem = (Engine::ScriptSystem*)manager.GetSystem("Script");
	m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");
	m_creatures = (CreatureSystem*)manager.GetSystem("Creatures");

	m_sceneEditor.Init(&m_scene, m_debugGui);

	auto& fileMenu = g_menuBar.AddSubmenu(ICON_FK_FILE_O " File");
	fileMenu.AddItem("Exit", []() { g_keepRunning = false; });

	auto& scriptMenu = g_menuBar.AddSubmenu(ICON_FK_GLOBE " Scene");
	scriptMenu.AddItem("New Scene", [this]() { NewScene(); });
	scriptMenu.AddItem("Load Scene", [this] { 
		std::string sceneFile = Engine::ShowFilePicker("Load Scene", "", "Scene Files (.scene)\0*.scene\0");
		LoadScene(sceneFile);
	});
	scriptMenu.AddItem("Save Scene", [this] {
		std::string sceneFile = Engine::ShowFilePicker("Save Scene", "", "Scene Files (.scene)\0*.scene\0", true);
		SaveScene(sceneFile);
	});
	scriptMenu.AddItem("Toggle Editor", [this] { m_sceneEditor.ToggleEnabled(); });
	scriptMenu.AddItem("Toggle Paused", [] { g_pauseScriptDelta = !g_pauseScriptDelta; });

	return true;
}

bool Playground::PostInit()
{
	LoadConfig();
	if (g_playgroundConfig.m_lastLoadedScene.length() > 0)
	{
		LoadScene(g_playgroundConfig.m_lastLoadedScene);
	}

	auto scripts = m_scriptSystem->Globals()["Playground"].get_or_create<sol::table>();
	scripts["ReloadScripts"] = [this]() {
		m_reloadScripts = true;
	};

	return true;
}

void Playground::ShowSystemProfiler()
{
	bool open = true;
	m_debugGui->BeginWindow(open, "System Profiler");
	char text[256] = "";
	double totalTime = 0.0;
	for (const auto& it : m_systemManager->GetLastUpdateTimes())
	{
		totalTime += std::get<1>(it);
		sprintf_s(text, "%s: %3.3fms", std::get<0>(it).c_str(), (float)std::get<1>(it) * 1000.0f);
		m_debugGui->Text(text);
	}
	sprintf_s(text, "Total: %3.3fms", (float)totalTime * 1000.0f);
	m_debugGui->Text(text);
	m_debugGui->EndWindow();
}

bool Playground::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	m_debugGui->MainMenuBar(g_menuBar);
	ShowSystemProfiler();

	if (m_sceneEditor.Tick() || m_reloadScripts)
	{
		ReloadScripts();
		m_reloadScripts = false;
	}

	if (g_pauseScriptDelta)
	{
		timeDelta = 0.0f;
	}
	TickScene(timeDelta);

	return g_keepRunning;
}

void Playground::Shutdown()
{
	SDE_PROF_EVENT();
	m_loadedSceneScripts.clear();
}