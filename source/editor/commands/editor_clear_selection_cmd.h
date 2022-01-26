#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <set>

class EditorClearSelectionCommand : public Command
{
public:
	virtual const char* GetName() { return "Clear Selection"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	std::set<EntityHandle> m_oldSelection;
};