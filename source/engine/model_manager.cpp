#include "model_manager.h"
#include "model.h"
#include "model_asset.h"
#include "texture_manager.h"
#include "job_system.h"
#include "debug_gui_system.h"
#include "core/profiler.h"
#include "core/thread.h"
#include "render/device.h"

namespace Engine
{
	ModelManager::ModelManager(TextureManager* tm, JobSystem* js)
		: m_textureManager(tm)
		, m_jobSystem(js)
	{
	}

	bool ModelManager::ShowGui(DebugGuiSystem& gui)
	{
		SDE_PROF_EVENT();

		static bool s_showWindow = true;
		gui.BeginWindow(s_showWindow, "ModelManager");
		char text[1024] = { '\0' };
		int32_t inFlight = m_inFlightModels;
		sprintf_s(text, "Loading: %d", inFlight);
		gui.Text(text);
		gui.Separator();
		for (int t = 0; t < m_models.size(); ++t)
		{
			sprintf_s(text, "%d: %s (0x%p)", t, m_models[t].m_name.c_str(), m_models[t].m_model.get());
			gui.Text(text);
		}
		gui.EndWindow();
		return s_showWindow;
	}

	void ModelManager::ReloadAll()
	{
		SDE_PROF_EVENT();

		// wait until all jobs finish, not great but eh
		while (m_inFlightModels > 0)
		{
			int v = m_inFlightModels;
			Core::Thread::Sleep(1);
		}
		// clear out the old results
		{
			Core::ScopedMutex guard(m_loadedModelsMutex);
			m_loadedModels.clear();
		}
		// now load the models again
		auto currentModels = std::move(m_models);
		for (int m = 0; m < currentModels.size(); ++m)
		{
			auto newHandle = LoadModel(currentModels[m].m_name.c_str());
			assert(m == newHandle.m_index);
		}
	}

	// this must be called on main thread before rendering!
	void ModelManager::FinaliseModel(Assets::Model& model, Model& renderModel, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders)
	{
		SDE_PROF_EVENT();
		const auto meshCount = renderModel.Parts().size();
		for (int index = 0; index < meshCount; ++index)
		{
			if (renderModel.Parts()[index].m_mesh != nullptr)
			{
				const auto& mesh = model.Meshes()[index];
				const auto& mat = mesh.Material();

				// load textures and store in mesh material
				std::string diffusePath = mat.DiffuseMaps().size() > 0 ? mat.DiffuseMaps()[0] : "";
				std::string normalPath = mat.NormalMaps().size() > 0 ? mat.NormalMaps()[0] : "";
				std::string specPath = mat.SpecularMaps().size() > 0 ? mat.SpecularMaps()[0] : "";
				auto& material = renderModel.Parts()[index].m_mesh->GetMaterial();
				material.SetSampler("DiffuseTexture", m_textureManager->LoadTexture(diffusePath.c_str()).m_index);
				material.SetSampler("NormalsTexture", m_textureManager->LoadTexture(normalPath.c_str()).m_index);
				material.SetSampler("SpecularTexture", m_textureManager->LoadTexture(specPath.c_str()).m_index);

				// Create render resources that cannot be shared across contexts
				meshBuilders[index]->CreateVertexArray(*renderModel.Parts()[index].m_mesh);
			}
		}
	}

	std::unique_ptr<Model> ModelManager::CreateModel(Assets::Model& model, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders)
	{
		SDE_PROF_EVENT();

		auto resultModel = std::make_unique<Model>();
		const auto& builder = meshBuilders.begin();

		const auto meshCount = model.Meshes().size();
		for (int index=0;index<meshCount;++index)
		{
			const auto& mesh = model.Meshes()[index];

			auto newMesh = std::make_unique<Render::Mesh>();
			meshBuilders[index]->CreateMesh(*newMesh);

			// materials pushed to uniform buffers
			const auto& mat = mesh.Material();
			auto& uniforms = newMesh->GetMaterial().GetUniforms();
			auto packedSpecular = glm::vec4(mat.SpecularColour(), mat.ShininessStrength());
			uniforms.SetValue("MeshDiffuseOpacity", glm::vec4(mat.DiffuseColour(), mat.Opacity()));
			uniforms.SetValue("MeshSpecular", packedSpecular);
			uniforms.SetValue("MeshShininess", mat.Shininess());

			Model::Part newPart;
			newPart.m_mesh = std::move(newMesh);
			newPart.m_transform = mesh.Transform();
			newPart.m_boundsMin = mesh.BoundsMin();
			newPart.m_boundsMax = mesh.BoundsMax();
			resultModel->Parts().push_back(std::move(newPart));
		}

		// Ensure any writes are shared with all contexts
		Render::Device::FlushContext();

		return resultModel;
	}

	void ModelManager::ProcessLoadedModels()
	{
		SDE_PROF_EVENT();
		
		std::vector<ModelLoadResult> loadedModels;
		{
			Core::ScopedMutex guard(m_loadedModelsMutex);
			loadedModels = std::move(m_loadedModels);
		}

		for (auto& loadedModel : loadedModels)
		{
			assert(loadedModel.m_destinationHandle.m_index != -1);
			if (loadedModel.m_renderModel != nullptr)
			{
				FinaliseModel(*loadedModel.m_model, *loadedModel.m_renderModel, loadedModel.m_meshBuilders);
				m_models[loadedModel.m_destinationHandle.m_index].m_model = std::move(loadedModel.m_renderModel);
			}
		}
	}

	std::unique_ptr<Render::MeshBuilder> ModelManager::CreateBuilderForPart(const Assets::ModelMesh& mesh)
	{
		SDE_PROF_EVENT();

		auto builder = std::make_unique<Render::MeshBuilder>();
		builder->AddVertexStream(3, mesh.Indices().size());		// position
		builder->AddVertexStream(3, mesh.Indices().size());		// normal
		builder->AddVertexStream(3, mesh.Indices().size());		// tangents
		builder->AddVertexStream(2, mesh.Indices().size());		// uv
		builder->BeginChunk();
		const auto& vertices = mesh.Vertices();
		const auto& indices = mesh.Indices();
		{
			SDE_PROF_EVENT("SetStreamData");
			for (uint32_t index = 0; index < indices.size(); index += 3)
			{
				const auto& v0 = vertices[indices[index]];
				const auto& v1 = vertices[indices[index + 1]];
				const auto& v2 = vertices[indices[index + 2]];
				builder->BeginTriangle();
				builder->SetStreamData(0, v0.m_position, v1.m_position, v2.m_position);
				builder->SetStreamData(1, v0.m_normal, v1.m_normal, v2.m_normal);
				builder->SetStreamData(2, v0.m_tangent, v1.m_tangent, v2.m_tangent);
				builder->SetStreamData(3, v0.m_texCoord0, v1.m_texCoord0, v2.m_texCoord0);
				builder->EndTriangle();
			}
		}
		builder->EndChunk();
		return builder;
	}

	ModelHandle ModelManager::LoadModel(const char* path)
	{
		SDE_PROF_EVENT();

		for (uint64_t i = 0; i < m_models.size(); ++i)
		{
			if (m_models[i].m_name == path)
			{
				return { static_cast<uint16_t>(i) };
			}
		}

		// always make a valid handle
		m_models.push_back({nullptr, path });
		auto newHandle = ModelHandle{ static_cast<uint16_t>(m_models.size() - 1) };
		m_inFlightModels += 1;

		std::string pathString = path;
		m_jobSystem->PushJob([this, pathString, newHandle](void*) {
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "LoadModel %s", pathString.c_str());
			SDE_PROF_EVENT_DYN(debugName);

			auto loadedAsset = Assets::Model::Load(pathString.c_str());
			if (loadedAsset != nullptr)
			{
				std::vector<std::unique_ptr<Render::MeshBuilder>> meshBuilders;
				for (const auto& part : loadedAsset->Meshes())
				{
					meshBuilders.push_back(CreateBuilderForPart(part));
				}

				auto theModel = CreateModel(*loadedAsset, meshBuilders);	// this does not create VAOs as they cannot be shared across contexts
				{
					Core::ScopedMutex guard(m_loadedModelsMutex);
					m_loadedModels.push_back({ std::move(loadedAsset), std::move(theModel), std::move(meshBuilders), newHandle });
					m_inFlightModels -= 1;
				}
			}
		});
		
		return newHandle;
	}

	std::string ModelManager::GetModelPath(const ModelHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_models.size())
		{
			return m_models[h.m_index].m_name;
		}
		else
		{
			return "<Empty Handle>";
		}
	}

	Model* ModelManager::GetModel(const ModelHandle& h)
	{
		if (h.m_index != -1 && h.m_index < m_models.size())
		{
			return m_models[h.m_index].m_model.get();
		}
		else
		{
			return nullptr;
		}
	}
}