#include "editor.h"
#include "core/log.h"
#include "core/file_io.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "entity/entity_system.h"
#include "entity/component_storage.h"
#include "commands/editor_close_cmd.h"
#include "commands/editor_new_scene_cmd.h"
#include "commands/editor_save_scene_cmd.h"
#include "engine/serialisation.h"

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::NewScene(const char* sceneName)
{
	m_sceneName = sceneName;
	m_sceneFilepath = "";
	m_entitySystem->NewWorld();
}

void Editor::StopRunning()
{
	m_keepRunning = false;
}

bool Editor::PreInit(Engine::SystemManager& manager)
{
	SDE_PROF_EVENT();
	m_debugGui = (Engine::DebugGuiSystem*)manager.GetSystem("DebugGui");
	m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");
	
	return true;
}

bool Editor::PostInit()
{
	SDE_PROF_EVENT();
	return true;
}

bool Editor::SaveScene(const char* fileName)
{
	SDE_PROF_EVENT();

	World* world = m_entitySystem->GetWorld();
	std::vector<uint32_t> allEntities = world->AllEntities();

	nlohmann::json json;
	Engine::ToJson("SceneName", m_sceneName, json);
	
	const uint32_t entityCount = allEntities.size();
	Engine::ToJson("EntityCount", entityCount, json);
	Engine::ToJson("EntityIDs", allEntities, json);

	std::vector<nlohmann::json> allEntityData;
	allEntityData.reserve(entityCount);
	for (int index=0;index<entityCount;++index)
	{
		auto& entityJson = allEntityData.emplace_back();
		Engine::ToJson("ID", allEntities[index], entityJson);

		std::vector<ComponentType> cmpTypes = world->GetOwnedComponentTypes(allEntities[index]);
		std::vector<nlohmann::json> componentData;
		componentData.reserve(cmpTypes.size());
		for (const auto& type : cmpTypes)
		{
			ComponentStorage* storage = world->GetStorage(type);
			if (storage)
			{
				storage->Serialise(allEntities[index], componentData.emplace_back(), Engine::SerialiseType::Write);
			}
		}
		entityJson["Components"] = componentData;
	}
	json["Entities"] = allEntityData;

	bool result = Core::SaveTextToFile(fileName, json.dump(2));
	if (result)
	{
		m_sceneFilepath = fileName;
	}
	return result;
}

void Editor::UpdateMenubar()
{
	Engine::MenuBar menuBar;
	auto& fileMenu = menuBar.AddSubmenu(ICON_FK_FILE_O " File");
	fileMenu.AddItem("Exit", [this]() {
		m_commands.Push(std::make_unique<EditorCloseCommand>(m_debugGui, this));
	});

	auto& scenesMenu = menuBar.AddSubmenu(ICON_FK_FILE_CODE_O " Scenes");
	scenesMenu.AddItem("New Scene", [this]() {
		m_commands.Push(std::make_unique<EditorNewSceneCommand>(m_debugGui, this));
	});
	scenesMenu.AddItem("Save Current Scene", [this]() {
		m_commands.Push(std::make_unique<EditorSaveSceneCommand>(m_debugGui, this, m_sceneFilepath));
	});

	m_debugGui->MainMenuBar(menuBar);
}

bool Editor::Tick(float timeDelta)
{
	SDE_PROF_EVENT();
	UpdateMenubar();

	m_commands.RunNext();

	return m_keepRunning;
}

void Editor::Shutdown()
{
	SDE_PROF_EVENT();
}