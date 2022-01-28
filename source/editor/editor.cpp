#include "editor.h"
#include "core/log.h"
#include "core/file_io.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/physics_system.h"
#include "engine/debug_render.h"
#include "engine/shader_manager.h"
#include "engine/file_picker_dialog.h"
#include "engine/graphics_system.h"
#include "engine/input_system.h"
#include "entity/entity_system.h"
#include "entity/component_storage.h"
#include "commands/editor_close_cmd.h"
#include "commands/editor_new_scene_cmd.h"
#include "commands/editor_save_scene_cmd.h"
#include "commands/editor_import_scene_cmd.h"
#include "commands/editor_create_entity_from_mesh_cmd.h"
#include "commands/editor_select_all_cmd.h"
#include "commands/editor_clear_selection_cmd.h"
#include "commands/editor_delete_selection_cmd.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_model.h"
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
	m_sceneFilepath = "";
	m_entitySystem->NewWorld();
	ImportScene("editor/empty_scene_template.scn");
	m_sceneName = sceneName;
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

	m_entitySystem->SerialiseEntities(sceneJson);

	return true;
}

bool Editor::SaveScene(const char* fileName)
{
	SDE_PROF_EVENT();

	World* world = m_entitySystem->GetWorld();
	std::vector<uint32_t> allEntities = world->AllEntities();

	nlohmann::json json = m_entitySystem->SerialiseEntities(allEntities);
	Engine::ToJson("SceneName", m_sceneName, json);

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
	if (m_selectedEntities.size() > 0)
	{
		editMenu.AddItem(ICON_FK_TRASH " Delete Selection", [this]() {
			m_commands.Push(std::make_unique<EditorDeleteSelectionCommand>());
		});
	}

	m_debugGui->MainMenuBar(menuBar);

	Engine::SubMenu rightClickBar;
	rightClickBar.m_label = "Options";
	auto& entityMenu = rightClickBar.AddSubmenu("Add Entity");
	entityMenu.AddItem("From Mesh", [this]() {
		std::string newFile = Engine::ShowFilePicker("Select Model", "", "Model Files (.fbx)\0*.fbx\0(.obj)\0*.obj\0");
		if (newFile != "")
		{
			m_commands.Push(std::make_unique<EditorCreateEntityFromMeshCommand>(newFile));
		}
	});
	if (m_selectedEntities.size() > 0)
	{
		rightClickBar.AddItem(ICON_FK_TRASH " Delete Selection", [this]() {
			m_commands.Push(std::make_unique<EditorDeleteSelectionCommand>());
		});
	}
	m_debugGui->ContextMenuVoid(rightClickBar);
}

void Editor::DrawGrid(float cellSize, int cellCount, glm::vec4 colour)
{
	auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	for (int x = -cellCount; x < cellCount; ++x)
	{
		graphics->DebugRenderer().DrawLine({ x * cellSize, 0.0f, -cellCount * cellSize }, { x * cellSize, 0.0f, cellCount * cellSize }, colour);
		for (int z = -cellCount; z < cellCount; ++z)
		{
			graphics->DebugRenderer().DrawLine({ -cellCount * cellSize, 0.0f, z * cellSize }, { cellCount * cellSize, 0.0f, z * cellSize }, colour);
		}
	}
}

bool Editor::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto mm = Engine::GetSystem<Engine::ModelManager>("Models");
	auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	auto world = m_entitySystem->GetWorld();

	UpdateMenubar();

	bool keepOpen = true;
	if (m_debugGui->BeginWindow(keepOpen, "Grid Settings"))
	{
		m_showGrid = m_debugGui->Checkbox("Show Grid", m_showGrid);
		m_gridSize = m_debugGui->DragFloat("Grid Size", m_gridSize, 0.25f, 0.25f, 8192.0f);
		m_gridCount = m_debugGui->DragInt("Grid Lines", m_gridCount, 1, 1024);
		m_gridColour = m_debugGui->ColourEdit("Colour", m_gridColour);
		m_debugGui->EndWindow();
	}

	if (m_showGrid)
	{
		DrawGrid(m_gridSize, m_gridCount, m_gridColour);
	}
	
	for (auto sel : m_selectedEntities)
	{
		auto modelCmp = world->GetComponent<Model>(sel);
		auto transformCmp = world->GetComponent<Transform>(sel);
		if (modelCmp && transformCmp)
		{
			auto theModel = mm->GetModel(modelCmp->GetModel());
			if (theModel)
			{
				graphics->DrawModelBounds(*theModel, transformCmp->GetMatrix(), glm::vec4(1.0f,1.0f,0.0f,1.0f), glm::vec4(0.0f, 1.0f, 1.0f, 0.05f));
			}
		}
	}

	if (!m_debugGui->IsCapturingKeyboard())
	{
		auto input = Engine::GetSystem<Engine::InputSystem>("Input");
		auto keyPressed = input->GetKeyboardState().m_keyPressed;
		static float s_ignoreKeyTimer = 0.0f;
		s_ignoreKeyTimer -= timeDelta;
		if (s_ignoreKeyTimer < 0.0f)	// throttle commands
		{
			s_ignoreKeyTimer = 0.1f;
			if (keyPressed[Engine::Key::KEY_LCTRL] && keyPressed[Engine::Key::KEY_z] && m_commands.CanUndo())
			{
				m_commands.Undo();
			}
			else if (keyPressed[Engine::Key::KEY_LCTRL] && keyPressed[Engine::Key::KEY_y] && m_commands.CanRedo())
			{
				m_commands.Redo();
			}
			else if (keyPressed[Engine::Key::KEY_LCTRL] && keyPressed[Engine::Key::KEY_a])
			{
				m_commands.Push(std::make_unique<EditorSelectAllCommand>());
			}
			else
			{
				s_ignoreKeyTimer = 0.0f;
			}
		}
	}

	m_commands.RunNext();

	return m_keepRunning;
}

void Editor::Shutdown()
{
	SDE_PROF_EVENT();
}