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
			int labelIndex = 0;
			int32_t inFlight = m_inFlightModels;
			sprintf_s(text, "Loading: %d", inFlight);
			gui.Text(text);
			gui.Separator();
			auto& textures = *Engine::GetSystem<Engine::TextureManager>("Textures");
			if (gui.TreeNode("All Models", true))
			{
				for (int t = 0; t < m_models.size(); ++t)
				{
					sprintf_s(text, "%s", m_models[t].m_name.c_str());
					if (m_models[t].m_renderModel.get() && gui.TreeNode(text))
					{
						auto imguiLabel = [&](const char* lbl) {
							sprintf_s(text, "%s##%d", lbl, labelIndex++);
							return text;
						};
						auto doTexture = [&](std::string name, Engine::TextureHandle& t)
						{
							auto theTexture = textures.GetTexture(t);
							auto path = textures.GetTexturePath(t);
							if (gui.Button(path.length() > 0 ? path.c_str() : name.c_str()))
							{
								std::string newFile = Engine::ShowFilePicker("Select Texture", "", "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png\0BMP (.bmp)\0*.bmp\0");
								if (newFile != "")
								{
									t = textures.LoadTexture(newFile.c_str());
								}
							}
							if (theTexture)
							{
								gui.Image(*theTexture, { 256,256 });
							}
						};
						auto& parts = m_models[t].m_renderModel->MeshParts();
						for (auto& p : parts)
						{
							sprintf_s(text, "%d: (%3.1f,%3.1f,%3.1f) - (%3.1f,%3.1f,%3.1f)", (int)(&p - parts.data()),
								p.m_boundsMin.x, p.m_boundsMin.y, p.m_boundsMin.z,
								p.m_boundsMax.x, p.m_boundsMax.y, p.m_boundsMax.z);
							if (gui.TreeNode(text))
							{
								p.m_drawData.m_castsShadows = gui.Checkbox(imguiLabel("Cast Shadows"), p.m_drawData.m_castsShadows);
								p.m_drawData.m_isTransparent = gui.Checkbox(imguiLabel("Transparent"), p.m_drawData.m_isTransparent);
								p.m_drawData.m_diffuseOpacity = gui.ColourEdit(imguiLabel("Diffuse/Opacity"), p.m_drawData.m_diffuseOpacity);
								p.m_drawData.m_specular = gui.ColourEdit(imguiLabel("Specular"), p.m_drawData.m_specular);
								p.m_drawData.m_shininess.x = gui.DragFloat(imguiLabel("Shininess"), p.m_drawData.m_shininess.x);
								doTexture(imguiLabel("Diffuse Texture"), p.m_drawData.m_diffuseTexture);
								doTexture(imguiLabel("Normals Texture"), p.m_drawData.m_diffuseTexture);
								doTexture(imguiLabel("Specular Texture"), p.m_drawData.m_diffuseTexture);
								gui.TreePop();
							}
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

			auto packedSpecular = glm::vec4(mat.SpecularColour(), mat.ShininessStrength());
			auto& dd = renderModel.MeshParts()[p].m_drawData;
			dd.m_diffuseOpacity = glm::vec4(mat.DiffuseColour(), mat.Opacity());
			dd.m_specular = packedSpecular;
			dd.m_shininess.r = mat.Shininess();
			dd.m_diffuseTexture = tm.LoadTexture(diffusePath.c_str());
			dd.m_normalsTexture = tm.LoadTexture(normalPath.c_str());
			dd.m_specularTexture = tm.LoadTexture(specPath.c_str());
			dd.m_castsShadows = true;
			dd.m_isTransparent = mat.Opacity() < 1.0f;
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

	bool ModelManager::Initialise()
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