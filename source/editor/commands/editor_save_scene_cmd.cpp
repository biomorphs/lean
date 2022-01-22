#include "editor_save_scene_cmd.h"
#include "engine/debug_gui_system.h"
#include "engine/file_picker_dialog.h"
#include "editor/editor.h"

Command::Result EditorSaveSceneCommand::Execute()
{
	std::string targetPath = m_targetPath;
	if (targetPath == "")
	{
		targetPath = Engine::ShowFilePicker("Save Scene", "", "Scene Files (.scn)\0*.scn\0", true);
	}

	bool success = m_editor->SaveScene(targetPath.c_str());
	return success ? Command::Result::Succeeded : Command::Result::Failed;
}