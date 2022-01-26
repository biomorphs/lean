#pragma once

#include "engine/serialisation.h"
#include "entity/entity_handle.h"
#include "editor/command.h"
#include <set>

class EditorDeleteSelectionCommand : public Command
{
public:
	virtual const char* GetName() { return "Delete Selected Entities"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	std::set<EntityHandle> m_oldSelection;
	nlohmann::json m_selectedEntityData;	// selection is serialised so all components are restored
};