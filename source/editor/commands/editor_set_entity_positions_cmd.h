#pragma once

#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "editor/command.h"
#include <vector>

class EditorSetEntityPositionsCommand : public Command
{
public:
	virtual const char* GetName() { return "Move Entities"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo();
	virtual Result Redo();
	void AddEntity(EntityHandle h, glm::vec3 oldPos, glm::vec3 newPos) { m_entities.push_back({ h, oldPos, newPos }); }

private:
	struct EntityPos
	{
		EntityHandle m_entity;
		glm::vec3 m_oldPos;
		glm::vec3 m_newPos;
	};
	std::vector<EntityPos> m_entities;
};