#include "editor_set_entity_positions_cmd.h"
#include "engine/system_manager.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_physics.h"

Command::Result EditorSetEntityPositionsCommand::Redo()
{
	return Execute();
}

Command::Result EditorSetEntityPositionsCommand::Undo()
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	for (auto& ent : m_entities)
	{
		auto transform = entities->GetWorld()->GetComponent<Transform>(ent.m_entity);
		if (transform)
		{
			transform->SetPosition(ent.m_oldPos);
		}
		auto physics = entities->GetWorld()->GetComponent<Physics>(ent.m_entity);
		if (physics)
		{
			physics->Rebuild();
		}
	}
	return Command::Result::Succeeded;
}

Command::Result EditorSetEntityPositionsCommand::Execute()
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	for (auto& ent : m_entities)
	{
		auto transform = entities->GetWorld()->GetComponent<Transform>(ent.m_entity);
		if (transform)
		{
			transform->SetPosition(ent.m_newPos);
		}
		auto physics = entities->GetWorld()->GetComponent<Physics>(ent.m_entity);
		if (physics)
		{
			physics->Rebuild();
		}
	}
	return Command::Result::Succeeded;
}