#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <vector>

class EditorSelectAllCommand : public Command
{
public:
	virtual const char* GetName() { return "Select All"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	std::vector<EntityHandle> m_oldSelection;
};