#include "editor_close_cmd.h"
#include "engine/debug_gui_system.h"
#include "editor/editor.h"

Command::Result EditorCloseCommand::Execute()
{
	Command::Result commandResult = Command::Result::Waiting;
	if (m_gui->BeginModalPopup("Close Editor?"))
	{	
		m_gui->Text("Are you sure you wish to quit? Unsaved changes will be lost");
		if (m_gui->Button("YES!"))
		{
			m_editor->StopRunning();
			m_gui->CloseCurrentPopup();
			commandResult = Command::Result::Succeeded;
		}
		m_gui->SameLine();
		if (m_gui->Button("NO!"))
		{
			m_gui->CloseCurrentPopup();
			commandResult = Command::Result::Failed;
		}
		m_gui->EndModalPopup();
	}
	return commandResult;
}