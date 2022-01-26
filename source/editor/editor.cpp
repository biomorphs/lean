#include "editor.h"
#include "core/log.h"
#include "core/file_io.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/physics_system.h"
#include "engine/shader_manager.h"
#include "engine/file_picker_dialog.h"
#include "entity/entity_system.h"
#include "entity/component_storage.h"
#include "commands/editor_close_cmd.h"
#include "commands/editor_new_scene_cmd.h"
#include "commands/editor_save_scene_cmd.h"
#include "commands/editor_import_scene_cmd.h"
#include "commands/editor_create_entity_from_mesh_cmd.h"
#include "commands/editor_select_all_cmd.h"
#include "commands/editor_clear_selection_cmd.h"
#include "engine/serialisation.h"

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::SelectEntity(EntityHandle h)
{
	m_selectedEntities.insert(h);
}

void Editor::DeselectEntity(EntityHandle h)
{
	m_selectedEntities.erase(h);
}

void Editor::DeselectAll()
{
	m_selectedEntities.clear();
}

void Editor::SelectAll()
{
	const auto& allEntities = Engine::GetSystem<EntitySystem>("Entities")->GetWorld()->AllEntities();
	m_selectedEntities.clear();
	for (const auto& e : allEntities)
	{
		m_selectedEntities.insert(e);
	}
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

bool Editor::PreInit()
{
	SDE_PROF_EVENT();
	m_debugGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	m_entitySystem = Engine::GetSystem<EntitySystem>("Entities");
	
	return true;
}

bool Editor::PostInit()
{
	SDE_PROF_EVENT();

	// Disable physics sim in editor by default
	Engine::GetSystem<Engine::PhysicsSystem>("Physics")->SetSimulationEnabled(false);

	// Todo Hacks to fix later 
	auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
	auto lightingShader = sm->LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs");
	auto shadowShader = sm->LoadShader("shadow", "simpleshadow.vs", "simpleshadow.fs");
	sm->SetShadowsShader(lightingShader, shadowShader);

	return true;
}

bool Editor::ImportScene(const char* fileName)
{
	SDE_PROF_EVENT();

	World* world = m_entitySystem->GetWorld();

	std::string sceneText;
	if (!Core::LoadTextFromFile(fileName, sceneText))
	{
		return false;
	}

	nlohmann::json sceneJson;
	{
		SDE_PROF_EVENT("ParseJson");
		sceneJson = nlohmann::json::parse(sceneText);
	}

	Engine::FromJson("SceneName", m_sceneName, sceneJson);

	uint32_t entityCount = 0;
	Engine::FromJson("EntityCount", entityCount, sceneJson);

	std::vector<uint32_t> oldEntityIDs;
	Engine::FromJson("EntityIDs", oldEntityIDs, sceneJson);
	if (oldEntityIDs.size() != entityCount)
	{
		SDE_LOG("Wha?");
		return false;
	}

	// First create the entities and build a remap table for the IDs
	std::unordered_map<uint32_t, uint32_t> oldEntityToNewEntity;
	{
		SDE_PROF_EVENT("AddEntities");
		oldEntityToNewEntity.reserve(oldEntityIDs.size());
		for (uint32_t oldID : oldEntityIDs)
		{
			oldEntityToNewEntity[oldID] = world->AddEntity().GetID();
		}
	}

	// Hook the OnLoaded callback of EntityHandle and remap any IDs we find during loading
	EntityHandle::OnLoaded remapId = [&oldEntityToNewEntity](EntityHandle& h) {
		if (h.GetID() != -1)
		{
			auto foundRemap = oldEntityToNewEntity.find(h.GetID());
			if (foundRemap != oldEntityToNewEntity.end())
			{
				h = EntityHandle(foundRemap->second);
			}
			else
			{
				SDE_LOG("Entity handle references ID that doesn't exist in the scene!");
			}
		}
	};
	EntityHandle::SetOnLoadFinishCallback(remapId);

	for (int index = 0; index < entityCount; ++index)
	{
		nlohmann::json& thisEntityData = sceneJson["Entities"][index];
		uint32_t oldId = 0;
		Engine::FromJson("ID", oldId, thisEntityData);		// this is the OLD id
		if (oldId != oldEntityIDs[index])
		{
			SDE_LOG("Wha?");
		}
		const uint32_t remappedId = oldEntityToNewEntity[oldId];
		auto& cmpList = thisEntityData["Components"];
		int componentCount = cmpList.size();
		for (int component = 0; component < componentCount; ++component)
		{
			std::string typeStr;
			nlohmann::json& cmpJson = cmpList[component];
			Engine::FromJson("Type", typeStr, cmpJson);

			ComponentStorage* storage = world->GetStorage(typeStr);
			if (storage)
			{
				nlohmann::json& dataJson = cmpJson["Data"];
				storage->Serialise(remappedId, dataJson, Engine::SerialiseType::Read);
			}
		}
	}

	EntityHandle::SetOnLoadFinishCallback(nullptr);

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
			nlohmann::json& cmpJson = componentData.emplace_back();
			cmpJson["Type"] = type;
			ComponentStorage* storage = world->GetStorage(type);
			nlohmann::json& dataJson = cmpJson["Data"];
			storage->Serialise(allEntities[index], dataJson, Engine::SerialiseType::Write);
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
	scenesMenu.AddItem("Import Scene", [this]() {
		m_commands.Push(std::make_unique<EditorImportSceneCommand>(this));
	});
	scenesMenu.AddItem("Make Entity From Mesh", [this]() {
		std::string newFile = Engine::ShowFilePicker("Select Model", "", "Model Files (.fbx)\0*.fbx\0(.obj)\0*.obj\0");
		if (newFile != "")
		{
			m_commands.Push(std::make_unique<EditorCreateEntityFromMeshCommand>(newFile));
		}
	});

	auto& editMenu = menuBar.AddSubmenu(ICON_FK_CLIPBOARD " Edit");
	if (m_commands.CanUndo())
	{
		editMenu.AddItem(ICON_FK_UNDO " Undo", [this]() {
			m_commands.Undo();
		});
	}
	if (m_commands.CanRedo())
	{
		editMenu.AddItem(ICON_FK_ARROW_CIRCLE_O_UP " Redo", [this]() {
			m_commands.Redo();
		});
	}
	editMenu.AddItem(ICON_FK_OBJECT_GROUP " Select All", [this] {
		m_commands.Push(std::make_unique<EditorSelectAllCommand>());
	});
	editMenu.AddItem(ICON_FK_MINUS_SQUARE " Clear Selection", [this] {
		m_commands.Push(std::make_unique<EditorClearSelectionCommand>());
	});

	m_debugGui->MainMenuBar(menuBar);
}

bool Editor::Tick(float timeDelta)
{
	SDE_PROF_EVENT();
	UpdateMenubar();

	m_commands.ShowWindow();
	m_commands.RunNext();

	return m_keepRunning;
}

void Editor::Shutdown()
{
	SDE_PROF_EVENT();
}