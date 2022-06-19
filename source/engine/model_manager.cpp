#include "model_manager.h"
#include "system_manager.h"
#include "model.h"
#include "model_asset.h"
#include "texture_manager.h"
#include "job_system.h"
#include "file_picker_dialog.h"
#include "debug_gui_system.h"
#include "debug_gui_menubar.h"
#include "core/profiler.h"
#include "core/thread.h"
#include "engine/system_manager.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/render_buffer.h"
#include "render/vertex_array.h"

namespace Engine
{
	const uint32_t c_maxVertices = 1024 * 1024 * 16;
	const uint32_t c_maxIndices = 1024 * 1024 * 64;

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

		static bool s_showWindow = false;
		auto& tm = *Engine::GetSystem<Engine::TextureManager>("Textures");

		Engine::MenuBar menuBar;
		auto& fileMenu = menuBar.AddSubmenu(ICON_FK_PAINT_BRUSH " Assets");
		fileMenu.AddItem("Model Manager", [this]() {
			s_showWindow = !s_showWindow;
			});
		gui.MainMenuBar(menuBar);

		if (s_showWindow)
		{
			gui.BeginWindow(s_showWindow, "ModelManager");
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
					if (m_models[t].m_renderModel.get() && gui.TreeNode(text))
					{
						if (gui.TreeNode("Parts"))
						{
							auto& parts = m_models[t].m_renderModel->MeshParts();
							for (auto& p : parts)
							{
								sprintf_s(text, "%d: (%3.1f,%3.1f,%3.1f) - (%3.1f,%3.1f,%3.1f)", (int)(&p - parts.data()),
									p.m_boundsMin.x, p.m_boundsMin.y, p.m_boundsMin.z,
									p.m_boundsMax.x, p.m_boundsMax.y, p.m_boundsMax.z);
								if (gui.TreeNode(text))
								{
									auto& material = p.m_material;
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
		}
		return true;
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
		// reset allocators
		m_nextVertex = 0;
		m_nextIndex = 0;

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
		
		int partCount = model.Meshes().size();
		for (int p = 0; p < partCount; ++p)
		{
			const auto& mat = model.Meshes()[p].Material();
			std::string diffusePath = mat.DiffuseMaps().size() > 0 ? mat.DiffuseMaps()[0] : "";
			std::string normalPath = mat.NormalMaps().size() > 0 ? mat.NormalMaps()[0] : "";
			std::string specPath = mat.SpecularMaps().size() > 0 ? mat.SpecularMaps()[0] : "";

			auto diffuseTexture = tm.LoadTexture(diffusePath.c_str());
			renderModel.MeshParts()[p].m_material.SetSampler("DiffuseTexture", diffuseTexture.m_index);
			renderModel.MeshParts()[p].m_material.SetSampler("NormalsTexture", tm.LoadTexture(normalPath.c_str()).m_index);
			renderModel.MeshParts()[p].m_material.SetSampler("SpecularTexture", tm.LoadTexture(specPath.c_str()).m_index);

			auto& uniforms = renderModel.MeshParts()[p].m_material.GetUniforms();
			auto packedSpecular = glm::vec4(mat.SpecularColour(), mat.ShininessStrength());
			uniforms.SetValue("MeshDiffuseOpacity", glm::vec4(mat.DiffuseColour(), mat.Opacity()));
			uniforms.SetValue("MeshSpecular", packedSpecular);
			uniforms.SetValue("MeshShininess", mat.Shininess());
			renderModel.MeshParts()[p].m_material.SetIsTransparent(mat.Opacity() != 1.0f);
		}
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
				m_models[loadedModel.m_destinationHandle.m_index].m_renderModel = std::move(loadedModel.m_renderModel);
				if (loadedModel.m_onFinish != nullptr)
				{
					loadedModel.m_onFinish(true, loadedModel.m_destinationHandle);
				}
			}
		}
	}

	uint32_t ModelManager::AllocateIndices(uint32_t count)
	{
		uint64_t newIndex = m_nextIndex.fetch_add(count);
		return newIndex < c_maxIndices ? newIndex : -1;
	}

	uint32_t ModelManager::AllocateVertices(uint32_t count)
	{
		uint64_t newIndex = m_nextVertex.fetch_add(count);
		return newIndex < c_maxVertices ? newIndex : -1;
	}

	std::unique_ptr<Model> ModelManager::CreateNewModel(const Assets::Model& model)
	{
		SDE_PROF_EVENT();

		// we want to reserve one large block of vertex + index space for the entire model
		uint32_t totalIndices = 0, totalVertices = 0;
		const auto meshCount = model.Meshes().size();
		for (int index = 0; index < meshCount; ++index)
		{
			totalIndices += model.Meshes()[index].Indices().size();
			totalVertices += model.Meshes()[index].Vertices().size();
		}
		const uint32_t indexBufferOffset = AllocateIndices(totalIndices);
		const uint32_t vertexBufferOffset = AllocateVertices(totalVertices);
		std::vector<uint32_t> indicesToUpload;	// upload all parts in one go
		indicesToUpload.reserve(totalIndices);
		std::vector<Assets::MeshVertex> verticesToUpload;
		verticesToUpload.reserve(totalVertices);
		uint32_t currentIndexOffset = indexBufferOffset;
		uint32_t currentVertexOffset = vertexBufferOffset;
		auto resultModel = std::make_unique<Model>();
		for (int index = 0; index < meshCount; ++index)
		{
			const auto& loadedMesh = model.Meshes()[index];
			verticesToUpload.insert(verticesToUpload.end(), loadedMesh.Vertices().begin(), loadedMesh.Vertices().end());
			for (const auto i : loadedMesh.Indices())
			{
				indicesToUpload.push_back(currentVertexOffset + i);
			}
			
			Model::MeshPart newPart;
			newPart.m_transform = loadedMesh.Transform();
			newPart.m_boundsMin = loadedMesh.BoundsMin();
			newPart.m_boundsMax = loadedMesh.BoundsMax();
			Render::MeshChunk chunk { currentIndexOffset, (uint32_t)loadedMesh.Indices().size(), Render::PrimitiveType::Triangles };
			newPart.m_chunks.push_back(chunk);
			
			resultModel->MeshParts().push_back(std::move(newPart));

			currentIndexOffset += loadedMesh.Indices().size();
			currentVertexOffset += loadedMesh.Vertices().size();
		}
		m_globalIndexData->SetData(indexBufferOffset * sizeof(uint32_t), indicesToUpload.size() * sizeof(uint32_t), &indicesToUpload[0]);
		m_globalVertexData->SetData(vertexBufferOffset * sizeof(Assets::MeshVertex), verticesToUpload.size() * sizeof(Assets::MeshVertex), &verticesToUpload[0]);
	
		// Ensure any writes are shared with all contexts
		Render::Device::FlushContext();

		glm::vec3 aabbMin, aabbMax;
		model.CalculateAABB(aabbMin, aabbMax);
		resultModel->BoundsMin() = aabbMin;
		resultModel->BoundsMax() = aabbMax;

		return resultModel;
	}

	ModelHandle ModelManager::LoadModel(const char* path, std::function<void(bool, ModelHandle)> onFinish)
	{
		SDE_PROF_EVENT();

		for (uint64_t i = 0; i < m_models.size(); ++i)
		{
			if (m_models[i].m_name == path)
			{
				ModelHandle result = { static_cast<uint32_t>(i) };
				if (onFinish)
					onFinish(true, result);
				return result;
			}
		}

		// always make a valid handle
		m_models.push_back({nullptr, path });
		auto newHandle = ModelHandle{ static_cast<uint32_t>(m_models.size() - 1) };
		m_inFlightModels += 1;

		std::string pathString = path;
		GetSystem<JobSystem>("Jobs")->PushSlowJob([this, pathString, newHandle, onFinish](void*) {
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "LoadModel %s", pathString.c_str());
			SDE_PROF_EVENT_DYN(debugName);

			auto loadedAsset = Assets::Model::Load(pathString.c_str());
			if (loadedAsset != nullptr)
			{
				auto testModel = CreateNewModel(*loadedAsset);
				{
					Core::ScopedMutex guard(m_loadedModelsMutex);
					m_loadedModels.push_back({ std::move(loadedAsset), std::move(testModel), newHandle });
				}
			}
			else if (onFinish != nullptr)
			{
				onFinish(false, newHandle);
			}
			m_inFlightModels -= 1;
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
			return m_models[h.m_index].m_renderModel.get();
		}
		else
		{
			return nullptr;
		}
	}

	bool ModelManager::PostInit()
	{
		m_globalIndexData = std::make_unique<Render::RenderBuffer>();
		m_globalVertexData = std::make_unique<Render::RenderBuffer>();
		m_globalVertexArray = std::make_unique<Render::VertexArray>();

		const auto vertexSize = sizeof(Engine::Assets::MeshVertex);
		m_globalIndexData->Create(c_maxIndices * sizeof(uint32_t), Render::RenderBufferModification::Dynamic);
		m_globalVertexData->Create(c_maxVertices * vertexSize, Render::RenderBufferModification::Dynamic);

		auto vertexBuffer = m_globalVertexData.get();
		m_globalVertexArray->AddBuffer(0, vertexBuffer, Render::VertexDataType::Float, 3, 0, vertexSize);					// position
		m_globalVertexArray->AddBuffer(1, vertexBuffer, Render::VertexDataType::Float, 3, sizeof(float) * 3, vertexSize);	// normal
		m_globalVertexArray->AddBuffer(2, vertexBuffer, Render::VertexDataType::Float, 3, sizeof(float) * 6, vertexSize);	// tangents
		m_globalVertexArray->AddBuffer(3, vertexBuffer, Render::VertexDataType::Float, 2, sizeof(float) * 9, vertexSize);	// uv
		m_globalVertexArray->Create();

		return true;
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

		m_globalVertexArray = nullptr;
		m_globalVertexData = nullptr;
		m_globalIndexData = nullptr;
	}
}