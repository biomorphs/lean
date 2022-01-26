#include "editor_clear_selection_cmd.h"
#include "editor/editor.h"
#include "engine/system_manager.h"

Command::Result EditorClearSelectionCommand::Redo()
{
	return Execute();
}

Command::Result EditorClearSelectionCommand::Undo()
{
	auto editor = Engine::GetSystem<Editor>("Editor");
	for (auto h : m_oldSelection)
	{
		editor->SelectEntity(h);
	}
	return Command::Result::Succeeded;
}

Command::Result EditorClearSelectionCommand::Execute()
{
	auto editor = Engine::GetSystem<Editor>("Editor");
	m_oldSelection = editor->SelectedEntities();

	editor->DeselectAll();

	return Command::Result::Succeeded;
}