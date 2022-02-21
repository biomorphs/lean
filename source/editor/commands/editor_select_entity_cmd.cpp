#include "editor_select_entity_cmd.h"
#include "editor/editor.h"
#include "engine/system_manager.h"

Command::Result EditorSelectEntityCommand::Redo()
{
	return Execute();
}

Command::Result EditorSelectEntityCommand::Undo()
{
	SDE_PROF_EVENT();
	auto editor = Engine::GetSystem<Editor>("Editor");
	editor->DeselectAll();
	for (auto h : m_oldSelection)
	{
		editor->SelectEntity(h);
	}
	return Command::Result::Succeeded;
}

Command::Result EditorSelectEntityCommand::Execute()
{
	SDE_PROF_EVENT();
	auto editor = Engine::GetSystem<Editor>("Editor");
	m_oldSelection = editor->SelectedEntities();
	if (!m_appendSelection)
	{
		editor->DeselectAll();
	}
	editor->SelectEntity(m_newSelection);

	return Command::Result::Succeeded;
}