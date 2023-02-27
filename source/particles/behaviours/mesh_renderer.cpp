#include "mesh_renderer.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/file_picker_dialog.h"
#include "engine/renderer.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(MeshRenderer, RenderBehaviour)
	{
		SERIALISE_PROPERTY("Shader", m_shader);
		SERIALISE_PROPERTY("Model", m_model);
	}
	SERIALISE_END()

	void MeshRenderer::Draw(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		const uint32_t count = container.AliveParticles();
		if (count > 0)
		{
			graphics->Renderer().SubmitInstances(&container.Positions().GetValue(0), count, m_model, m_shader);
		}
	}

	void MeshRenderer::Inspect(EditorValueInspector& v)
	{
		auto models = Engine::GetSystem<Engine::ModelManager>("Models");
		auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");

		std::string modelPath = "Model: " + models->GetModelPath(m_model);
		if (v.m_dbgGui->Button(modelPath.c_str()))
		{
			std::string newFile = Engine::ShowFilePicker("Select Model", "", "Model Files (.fbx)\0*.fbx\0(.obj)\0*.obj\0(.gltf)\0*.gltf\0");
			if (newFile != "")
			{
				auto loadedModel = models->LoadModel(newFile.c_str());
				m_model = loadedModel;
			}
		}

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
		int shaderIndex = m_shader.m_index != (uint32_t)-1 ? m_shader.m_index : shaderpaths.size() - 1;
		if (v.m_dbgGui->ComboBox("Shader", shaderpaths, shaderIndex))
		{
			m_shader = { (uint32_t)(shaderIndex == (shaderpaths.size() - 1) ? (uint32_t)-1 : shaderIndex) };
		}
	}
}
