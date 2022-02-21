#include "editor_import_scene_cmd.h"
#include "engine/debug_gui_system.h"
#include "engine/file_picker_dialog.h"
#include "editor/editor.h"

Command::Result EditorImportSceneCommand::Execute()
{
	SDE_PROF_EVENT();
	std::string scenePath = Engine::ShowFilePicker("Import Scene", "", "Scene Files (.scn)\0*.scn\0");
	if (scenePath != "")
	{
		return m_editor->ImportScene(scenePath.c_str()) ? Command::Result::Succeeded : Command::Result::Failed;
	}
	return Command::Result::Failed;
}