#include "editor_clone_selection_cmd.h"
#include "editor/editor.h"
#include "entity/entity_system.h"
#include "engine/system_manager.h"

Command::Result EditorCloneSelectionCommand::Redo()
{
	return Execute();
}

Command::Result EditorCloneSelectionCommand::Undo()
{
	SDE_PROF_EVENT();
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto editor = Engine::GetSystem<Editor>("Editor");

	editor->DeselectAll();

	for (const auto it : m_newIDs)
	{
		entities->GetWorld()->RemoveEntity(it);
	}

	for (const auto it : m_oldSelection)
	{
		editor->SelectEntity(it);
	}

	m_newIDs.clear();
	m_oldSelection.clear();

	return Command::Result::Succeeded;
}

Command::Result EditorCloneSelectionCommand::Execute()
{
	SDE_PROF_EVENT();
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto editor = Engine::GetSystem<Editor>("Editor");

	const auto& selected = editor->SelectedEntities();
	std::vector<uint32_t> entityIDs;
	for (const auto it : selected)
	{
		entityIDs.push_back(it.GetID());
	}
	m_oldSelection = selected;
	editor->DeselectAll();

	// clone via serialisation
	nlohmann::json cloneData = entities->SerialiseEntities(entityIDs);
	m_newIDs = entities->SerialiseEntities(cloneData);

	// select the newly cloned entities for convenience
	for (const auto it : m_newIDs)
	{
		editor->SelectEntity(it);
	}

	return Command::Result::Succeeded;
}