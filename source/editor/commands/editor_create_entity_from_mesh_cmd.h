#pragma once

#include "entity/entity_handle.h"
#include "editor/command.h"
#include <string>

namespace Engine
{
	class ModelHandle;
}

class EditorCreateEntityFromMeshCommand : public Command
{
public:
	EditorCreateEntityFromMeshCommand(const std::string& meshPath)
		: m_meshPath(meshPath)
	{
	}
	virtual const char* GetName() { return "Create Entity From Mesh"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();

private:
	Result CreateEntity(const Engine::ModelHandle& mh);
	std::string m_meshPath = "";
	Command::Result m_currentStatus = Command::Result::Waiting;
	EntityHandle m_entityCreated;
};