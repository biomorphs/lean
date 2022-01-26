#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <set>

class EditorSelectAllCommand : public Command
{
public:
	virtual const char* GetName() { return "Select All"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	std::set<EntityHandle> m_oldSelection;
};