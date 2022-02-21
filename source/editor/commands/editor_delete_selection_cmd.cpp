#include "editor_delete_selection_cmd.h"
#include "editor/editor.h"
#include "entity/entity_system.h"
#include "engine/system_manager.h"

Command::Result EditorDeleteSelectionCommand::Redo()
{
	return Execute();
}

Command::Result EditorDeleteSelectionCommand::Undo()
{
	SDE_PROF_EVENT();

	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto editor = Engine::GetSystem<Editor>("Editor");

	std::vector<uint32_t> newIDs = entities->SerialiseEntities(m_selectedEntityData, true);
	for (const auto it : newIDs)
	{
		editor->SelectEntity(it);
	}

	return Command::Result::Succeeded;
}

Command::Result EditorDeleteSelectionCommand::Execute()
{
	SDE_PROF_EVENT();

	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto editor = Engine::GetSystem<Editor>("Editor");

	// keep the old selection around for later
	m_oldSelection = editor->SelectedEntities();
	
	// serialise the entities
	std::vector<uint32_t> entityIDs;
	for (const auto it : m_oldSelection)
	{
		entityIDs.push_back(it.GetID());
	}
	m_selectedEntityData = entities->SerialiseEntities(entityIDs);

	editor->DeselectAll();
	for (const auto it : m_oldSelection)
	{
		entities->GetWorld()->RemoveEntity(it);
	}

	return Command::Result::Succeeded;
}