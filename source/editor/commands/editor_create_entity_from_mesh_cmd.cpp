#include "editor_create_entity_from_mesh_cmd.h"
#include "engine/debug_gui_system.h"
#include "editor/editor.h"
#include "engine/model_manager.h"
#include "engine/shader_manager.h"
#include "engine/system_manager.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_model.h"
#include "engine/components/component_tags.h"

Command::Result EditorCreateEntityFromMeshCommand::Redo()
{
	return Execute();
}

Command::Result EditorCreateEntityFromMeshCommand::Undo()
{
	SDE_PROF_EVENT();
	Engine::GetSystem<EntitySystem>("Entities")->GetWorld()->RemoveEntity(m_entityCreated);
	return Command::Result::Succeeded;
}

Command::Result EditorCreateEntityFromMeshCommand::CreateEntity(const Engine::ModelHandle& mh)
{
	SDE_PROF_EVENT();

	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto world = entities->GetWorld();

	if (m_entityCreated.GetID() != -1)
	{
		m_entityCreated = world->AddEntityFromHandle(m_entityCreated);
		if (m_entityCreated.GetID() == -1)
		{
			SDE_LOG("Failed to recreate entity with ID %d", m_entityCreated.GetID());
			return Command::Result::Failed;
		}
	}
	else
	{
		m_entityCreated = world->AddEntity();
	}

	// todo... interface sucks
	world->AddComponent(m_entityCreated, "Transform");
	world->AddComponent(m_entityCreated, "Model");
	world->AddComponent(m_entityCreated, "Tags");
	Model* modelComponent = world->GetComponent<Model>(m_entityCreated);
	Transform* transComponent = world->GetComponent<Transform>(m_entityCreated);
	Tags* tagsComponent = world->GetComponent<Tags>(m_entityCreated);

	tagsComponent->AddTag(m_meshPath.c_str());

	// todo... parameter?
	auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
	auto theShader = shaders->LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs");

	modelComponent->SetModel(mh);
	modelComponent->SetShader(theShader);

	return Command::Result::Succeeded;
}

Command::Result EditorCreateEntityFromMeshCommand::Execute()
{
	SDE_PROF_EVENT();

	if (m_currentStatus == Command::Result::Failed)
	{
		return m_currentStatus;
	}

	auto modelManager = Engine::GetSystem<Engine::ModelManager>("Models");
	Engine::ModelHandle theModel = modelManager->LoadModel(m_meshPath.c_str(), [this](bool r, Engine::ModelHandle mh) {
		if (!r)
		{
			m_currentStatus = Command::Result::Failed;
		}
	});
	if (theModel.m_index != -1)
	{
		Engine::Model* modelPtr = modelManager->GetModel(theModel);
		if (modelPtr != nullptr)
		{
			m_currentStatus = CreateEntity(theModel);
		}
		else
		{
			m_currentStatus = Command::Result::Waiting;
		}
	}

	return m_currentStatus;
}