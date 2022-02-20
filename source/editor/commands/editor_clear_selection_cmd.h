#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <vector>

class EditorClearSelectionCommand : public Command
{
public:
	virtual const char* GetName() { return "Clear Selection"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	std::vector<EntityHandle> m_oldSelection;
};