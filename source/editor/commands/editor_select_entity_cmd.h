#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <set>

class EditorSelectEntityCommand : public Command
{
public:
	EditorSelectEntityCommand(EntityHandle h, bool appendToSelection=false) : m_newSelection(h), m_appendSelection(appendToSelection) {}
	virtual const char* GetName() { return "Select Entity"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	EntityHandle m_newSelection;
	bool m_appendSelection = false;
	std::set<EntityHandle> m_oldSelection;
};