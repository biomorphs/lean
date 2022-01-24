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

Command::Result EditorCreateEntityFromMeshCommand::Execute()
{
	if (m_currentStatus == Command::Result::Waiting)
	{
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
				auto entities = Engine::GetSystem<EntitySystem>("Entities");
				auto world = entities->GetWorld();

				m_entityCreated = world->AddEntity();

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

				modelComponent->SetModel(theModel);
				modelComponent->SetShader(theShader);
				
				m_currentStatus = Command::Result::Succeeded;
			}
		}
	}

	return m_currentStatus;
}