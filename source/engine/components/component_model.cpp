#include "component_model.h"
#include "component_material.h"
#include "entity/entity_system.h"
#include "engine/debug_gui_system.h"
#include "engine/model_manager.h"
#include "engine/shader_manager.h"
#include "engine/file_picker_dialog.h"
#include "engine/system_manager.h"

COMPONENT_SCRIPTS(Model,
	"SetShader", &Model::SetShader,
	"SetModel", &Model::SetModel,
	"SetMaterialEntity", &Model::SetMaterialEntity
)

SERIALISE_BEGIN(Model)
	SERIALISE_PROPERTY("MaterialEntity", m_materialEntity)
	SERIALISE_PROPERTY("Shader", m_shader)
	SERIALISE_PROPERTY("Model", m_model)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(Model, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<Model::StorageType&>(cs).Find(e);
		auto models = Engine::GetSystem<Engine::ModelManager>("Models");
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		std::string modelPath = models->GetModelPath(m.GetModel());
		if (gui.Button(modelPath.c_str()))
		{
			std::string newFile = Engine::ShowFilePicker("Select Model", "", "Model Files (.fbx)\0*.fbx\0(.obj)\0*.obj\0");
			if (newFile != "")
			{
				auto loadedModel = models->LoadModel(newFile.c_str());
				m.SetModel(loadedModel);
			}
		}
		auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		auto allShaders = shaders->AllShaders();
		std::vector<std::string> shaderpaths;
		std::string vs, fs;
		for (auto it : allShaders)
		{
			char shaderText[1024];
			if (shaders->GetShaderPaths(it, vs, fs))
			{
				sprintf(shaderText, "%s: %s, %s", shaders->GetShaderName(it).c_str(), vs.c_str(), fs.c_str());
				shaderpaths.push_back(shaderText);
			}
		}
		shaderpaths.push_back("None");
		int shaderIndex = m.GetShader().m_index != (uint32_t)-1 ? m.GetShader().m_index : shaderpaths.size() - 1;
		if (gui.ComboBox("Shader", shaderpaths, shaderIndex))
		{
			m.SetShader({ (uint32_t)(shaderIndex == (shaderpaths.size() - 1) ? (uint32_t)-1 : shaderIndex) });
		}

		std::string materialEntityName = m.GetMaterialEntity().GetID() != -1 ?
			entities->GetEntityNameWithTags(m.GetMaterialEntity()) : "None";
		materialEntityName = std::string("Material Entity: ") + materialEntityName;
		static EntityHandle setMatEntityHandle;
		if (gui.Button(materialEntityName.c_str()))
		{
			setMatEntityHandle = e;
		}

		if(setMatEntityHandle.GetID() != -1)
		{
			if (gui.BeginModalPopup("Select Material Entity"))
			{
				if (gui.Button("None"))
				{
					m.SetMaterialEntity({});
					setMatEntityHandle = {};
				}
				if (gui.Button("Myself"))
				{
					m.SetMaterialEntity(e);
					setMatEntityHandle = {};
				}
				entities->GetWorld()->ForEachComponent<Material>([entities, &gui, &m, &e](Material& mat, EntityHandle ent) {
					if (e.GetID() != ent.GetID())
					{
						std::string entityName = entities->GetEntityNameWithTags(ent);
						if (gui.Button(entityName.c_str()))
						{
							m.SetMaterialEntity(ent);
							setMatEntityHandle = {};
						}
					}
				});
				gui.EndModalPopup();
			}
		}
	};
	return fn;
}