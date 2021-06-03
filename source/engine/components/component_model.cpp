#include "component_model.h"
#include "engine/debug_gui_system.h"
#include "engine/model_manager.h"
#include "engine/shader_manager.h"
#include "engine/file_picker_dialog.h"

COMPONENT_SCRIPTS(Model,
	"SetShader", &Model::SetShader,
	"SetModel", &Model::SetModel,
	"SetMaterialEntity", &Model::SetMaterialEntity
)

COMPONENT_INSPECTOR_IMPL(Model, Engine::DebugGuiSystem& gui, Engine::ModelManager& models, Engine::ShaderManager& shaders)
{
	auto fn = [&gui,&models,&shaders](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<Model::StorageType&>(cs).Find(e);
		std::string modelPath = models.GetModelPath(m.GetModel());
		if (gui.Button(modelPath.c_str()))
		{
			std::string newFile = Engine::ShowFilePicker("Select Model", "", "Model Files (.fbx)\0*.fbx\0(.obj)\0*.obj\0");
			if (newFile != "")
			{
				auto loadedModel = models.LoadModel(newFile.c_str());
				m.SetModel(loadedModel);
			}
		}
		auto allShaders = shaders.AllShaders();
		std::vector<std::string> shaderpaths;
		std::string vs, fs;
		for (auto it : allShaders)
		{
			if (shaders.GetShaderPaths(it, vs, fs))
			{
				char shaderPathText[1024];
				sprintf(shaderPathText, "%s, %s", vs.c_str(), fs.c_str());
				shaderpaths.push_back(shaderPathText);
			}
		}
		shaderpaths.push_back("None");
		int shaderIndex = m.GetShader().m_index != (uint32_t)-1 ? m.GetShader().m_index : shaderpaths.size() - 1;
		if (gui.ComboBox("Shader", shaderpaths, shaderIndex))
		{
			m.SetShader({ (uint32_t)(shaderIndex == (shaderpaths.size() - 1) ? (uint32_t)-1 : shaderIndex) });
		}
	};
	return fn;
}