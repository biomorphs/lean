#include "editor_select_all_cmd.h"
#include "editor/editor.h"
#include "engine/system_manager.h"

Command::Result EditorSelectAllCommand::Redo()
{
	return Execute();
}

Command::Result EditorSelectAllCommand::Undo()
{
	auto editor = Engine::GetSystem<Editor>("Editor");
	editor->DeselectAll();
	for (auto h : m_oldSelection)
	{
		editor->SelectEntity(h);
	}
	return Command::Result::Succeeded;
}

Command::Result EditorSelectAllCommand::Execute()
{
	auto editor = Engine::GetSystem<Editor>("Editor");
	m_oldSelection = editor->SelectedEntities();

	editor->SelectAll();

	return Command::Result::Succeeded;
}