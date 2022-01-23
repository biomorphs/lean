#include "model_manager.h"
#include "system_manager.h"
#include "model.h"
#include "model_asset.h"
#include "texture_manager.h"
#include "job_system.h"
#include "file_picker_dialog.h"
#include "debug_gui_system.h"
#include "core/profiler.h"
#include "core/thread.h"
#include "engine/system_manager.h"
#include "render/device.h"
#include "render/mesh.h"

namespace Engine
{
	SERIALISE_BEGIN(ModelHandle)
		static ModelManager* mm = GetSystem<ModelManager>("Models");
		if (op == Engine::SerialiseType::Write)
		{
			Engine::ToJson("Path", mm->GetModelPath(*this), json);
		}
		else
		{
			std::string path = "";
			Engine::FromJson("Path", path, json);
			*this = mm->LoadModel(path.c_str());
		}
	SERIALISE_END()

	bool ModelManager::ShowGui(DebugGuiSystem& gui)
	{
		SDE_PROF_EVENT();

		auto& tm = *Engine::GetSystem<Engine::TextureManager>("Textures");

		bool showWindow = true;
		gui.BeginWindow(showWindow, "ModelManager");
		char text[1024] = { '\0' };
		int32_t inFlight = m_inFlightModels;
		sprintf_s(text, "Loading: %d", inFlight);
		gui.Text(text);
		gui.Separator();
		if (gui.TreeNode("All Models", true))
		{
			for (int t = 0; t < m_models.size(); ++t)
			{
				sprintf_s(text, "%s", m_models[t].m_name.c_str());
				if (m_models[t].m_model.get() && gui.TreeNode(text))
				{
					if (gui.TreeNode("Parts"))
					{
						auto& parts = m_models[t].m_model->Parts();
						for (auto& p : parts)
						{
							sprintf_s(text, "%d: (%3.1f,%3.1f,%3.1f) - (%3.1f,%3.1f,%3.1f)", (int)(&p - parts.data()),
								p.m_boundsMin.x, p.m_boundsMin.y, p.m_boundsMin.z,
								p.m_boundsMax.x, p.m_boundsMax.y, p.m_boundsMax.z);
							if(p.m_mesh && gui.TreeNode(text))
							{
								auto& material = p.m_mesh->GetMaterial();
								auto& uniforms = material.GetUniforms();
								auto& samplers = material.GetSamplers();
								for (auto& v : uniforms.FloatValues())
								{
									sprintf_s(text, "%s", v.second.m_name.c_str());
									v.second.m_value = gui.DragFloat(text, v.second.m_value);
								}
								for (auto& v : uniforms.Vec4Values())
								{
									sprintf_s(text, "%s", v.second.m_name.c_str());
									v.second.m_value = gui.DragVector(text, v.second.m_value);
								}
								for (auto& v : uniforms.IntValues())
								{
									sprintf_s(text, "%s", v.second.m_name.c_str());
									v.second.m_value = gui.DragInt(text, v.second.m_value);
								}
								for (auto& t : samplers)
								{
									sprintf_s(text, "%s", t.second.m_name.c_str());
									if (t.second.m_handle != 0 && gui.TreeNode(text))
									{
										auto texture = tm.GetTexture({ t.second.m_handle });
										auto path = tm.GetTexturePath({ t.second.m_handle });
										if (texture)
										{
											gui.Image(*texture, { 256,256 });
										}
										sprintf_s(text, "%s", t.second.m_name.c_str());
										if (gui.Button(text))
										{
											std::string newFile = Engine::ShowFilePicker("Select Texture", "", "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png\0BMP (.bmp)\0*.bmp\0");
											if (newFile != "")
											{
												auto loadedTexture = tm.LoadTexture(newFile.c_str());
												t.second.m_handle = loadedTexture.m_index;
											}
										}
										gui.TreePop();
									}
								}
								gui.TreePop();
							}
						}
						gui.TreePop();
					}

					gui.TreePop();
				}
			}
			gui.TreePop();
		}
		gui.EndWindow();
		return showWindow;
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
	void ModelManager::FinaliseModel(Assets::Model& model, Model& renderModel)
	{
		SDE_PROF_EVENT();
		auto& tm = *Engine::GetSystem<Engine::TextureManager>("Textures");
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
				// Set transparent flag based on diffuse texture
				auto diffuseTexture = tm.LoadTexture(diffusePath.c_str(), [this, &tm, &renderModel, index](bool loaded, TextureHandle h) {
					auto texture = tm.GetTexture(h);
					if (texture != nullptr && texture->GetComponentCount() == 4 && renderModel.Parts()[index].m_mesh != nullptr)
					{
						renderModel.Parts()[index].m_mesh->GetMaterial().SetIsTransparent(true);
					}
				});
				material.SetSampler("DiffuseTexture", diffuseTexture.m_index);
				material.SetSampler("NormalsTexture", tm.LoadTexture(normalPath.c_str()).m_index);
				material.SetSampler("SpecularTexture", tm.LoadTexture(specPath.c_str()).m_index);

				// Create render resources that cannot be shared across contexts
				renderModel.Parts()[index].m_mesh->GetVertexArray().Create();
			}
		}
	}

	std::unique_ptr<Model> ModelManager::CreateModel(Assets::Model& model, std::vector<std::unique_ptr<Render::Mesh>>& rendermeshes)
	{
		SDE_PROF_EVENT();
		assert(model.Meshes().size() == rendermeshes.size());

		auto resultModel = std::make_unique<Model>();
		const auto meshCount = model.Meshes().size();
		for (int index = 0; index < meshCount; ++index)
		{
			const auto& loadedMesh = model.Meshes()[index];
			Model::Part newPart;
			newPart.m_mesh = std::move(rendermeshes[index]);
			newPart.m_transform = loadedMesh.Transform();
			newPart.m_boundsMin = loadedMesh.BoundsMin();
			newPart.m_boundsMax = loadedMesh.BoundsMax();
			resultModel->Parts().push_back(std::move(newPart));
		}
		glm::vec3 aabbMin, aabbMax;
		model.CalculateAABB(aabbMin, aabbMax);
		resultModel->BoundsMin() = aabbMin;
		resultModel->BoundsMax() = aabbMax;

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
				FinaliseModel(*loadedModel.m_model, *loadedModel.m_renderModel);
				m_models[loadedModel.m_destinationHandle.m_index].m_model = std::move(loadedModel.m_renderModel);
			}
		}
	}

	std::unique_ptr<Render::Mesh> ModelManager::CreateMeshPart(const Assets::ModelMesh& mesh)
	{
		SDE_PROF_EVENT();
		auto rmesh = std::make_unique<Render::Mesh>();
		auto& streams = rmesh->GetStreams();
		const auto verts = mesh.Vertices().size();

		// we will add all vertex data to one big buffer!
		streams.push_back({});
		const auto vertexSize = sizeof(Engine::Assets::MeshVertex);
		if (!streams[0].Create((void*)mesh.Vertices().data(), vertexSize * verts, Render::RenderBufferModification::Static))
		{
			return nullptr;
		}

		// upload indices
		auto indexBuffer = std::make_unique<Render::RenderBuffer>();
		if (!indexBuffer->Create((void*)mesh.Indices().data(), sizeof(uint32_t) * mesh.Indices().size(), Render::RenderBufferModification::Static))
		{
			return nullptr;
		}
		rmesh->GetIndexBuffer() = std::move(indexBuffer);

		// Setup vertex array bindings (but dont create it here!)
		auto& va = rmesh->GetVertexArray();
		va.AddBuffer(0, &streams[0], Render::VertexDataType::Float, 3, 0, vertexSize);					// position
		va.AddBuffer(1, &streams[0], Render::VertexDataType::Float, 3, sizeof(float) * 3, vertexSize);	// normal
		va.AddBuffer(2, &streams[0], Render::VertexDataType::Float, 3, sizeof(float) * 6, vertexSize);	// tangents
		va.AddBuffer(3, &streams[0], Render::VertexDataType::Float, 2, sizeof(float) * 9, vertexSize);	// uv

		// Setup the per-mesh material uniforms (we can't load textures here though)
		const auto& mat = mesh.Material();
		auto& uniforms = rmesh->GetMaterial().GetUniforms();
		auto packedSpecular = glm::vec4(mat.SpecularColour(), mat.ShininessStrength());
		uniforms.SetValue("MeshDiffuseOpacity", glm::vec4(mat.DiffuseColour(), mat.Opacity()));
		uniforms.SetValue("MeshSpecular", packedSpecular);
		uniforms.SetValue("MeshShininess", mat.Shininess());
		rmesh->GetMaterial().SetIsTransparent(mat.Opacity() != 1.0f);

		rmesh->GetChunks().push_back({0, (uint32_t)mesh.Indices().size(), Render::PrimitiveType::Triangles});

		// Ensure any writes are shared with all contexts
		Render::Device::FlushContext();

		return rmesh;
	}

	ModelHandle ModelManager::LoadModel(const char* path)
	{
		SDE_PROF_EVENT();

		for (uint64_t i = 0; i < m_models.size(); ++i)
		{
			if (m_models[i].m_name == path)
			{
				return { static_cast<uint32_t>(i) };
			}
		}

		// always make a valid handle
		m_models.push_back({nullptr, path });
		auto newHandle = ModelHandle{ static_cast<uint32_t>(m_models.size() - 1) };
		m_inFlightModels += 1;

		std::string pathString = path;
		GetSystem<JobSystem>("Jobs")->PushSlowJob([this, pathString, newHandle](void*) {
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "LoadModel %s", pathString.c_str());
			SDE_PROF_EVENT_DYN(debugName);

			auto loadedAsset = Assets::Model::Load(pathString.c_str());
			if (loadedAsset != nullptr)
			{
				std::vector<std::unique_ptr<Render::Mesh>> renderMeshes;
				for (const auto& part : loadedAsset->Meshes())
				{
					renderMeshes.push_back(CreateMeshPart(part));
				}

				auto theModel = CreateModel(*loadedAsset, renderMeshes);	// this does not create VAOs as they cannot be shared across contexts
				{
					Core::ScopedMutex guard(m_loadedModelsMutex);
					m_loadedModels.push_back({ std::move(loadedAsset), std::move(theModel), newHandle });
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

	bool ModelManager::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		ProcessLoadedModels();
		ShowGui(*Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui"));

		return true;
	}

	void ModelManager::Shutdown()
	{
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
		m_models.clear();
	}
}