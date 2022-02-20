#pragma once

#include "engine/serialisation.h"
#include "entity/entity_handle.h"
#include "editor/command.h"
#include <vector>

class EditorCloneSelectionCommand : public Command
{
public:
	virtual const char* GetName() { return "Clone Selected Entities"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	std::vector<EntityHandle> m_oldSelection;
	std::vector<uint32_t> m_newIDs;
};