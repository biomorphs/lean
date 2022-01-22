#include "editor_new_scene_cmd.h"
#include "engine/debug_gui_system.h"
#include "editor/editor.h"

Command::Result EditorNewSceneCommand::Execute()
{
	Command::Result commandResult = Command::Result::Waiting;
	if (m_gui->BeginModalPopup("New Scene"))
	{	
		m_gui->TextInput("Scene Name", m_newSceneName);
		if (m_newSceneName.size() > 0 && m_gui->Button("Ok"))
		{
			m_editor->NewScene(m_newSceneName.c_str());
			m_gui->CloseCurrentPopup();
			commandResult = Command::Result::Succeeded;
		}
		m_gui->EndModalPopup();
	}
	return commandResult;
}