#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <string>

class EditorCreateEntityFromMeshCommand : public Command
{
public:
	EditorCreateEntityFromMeshCommand(const std::string& meshPath)
		: m_meshPath(meshPath)
	{
	}
	virtual const char* GetName() { return "Create Entity From Mesh"; }
	virtual Result Execute();
	virtual bool CanUndo() { return false; }	// todo

private:
	std::string m_meshPath = "";
	Command::Result m_currentStatus = Command::Result::Waiting;
	EntityHandle m_entityCreated;
};