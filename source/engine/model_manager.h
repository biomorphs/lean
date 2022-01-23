#pragma once
#include "model.h"
#include "model_asset.h"
#include "core/mutex.h"
#include "render/mesh_builder.h"
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
	class DebugGuiSystem;
	class JobSystem;

	struct ModelHandle
	{
		uint32_t m_index = -1;
		static ModelHandle Invalid() { return { (uint32_t)-1 }; };
	};

	class ModelManager
	{
	public:
		ModelManager(JobSystem* js);
		~ModelManager();
		ModelManager(const ModelManager&) = delete;
		ModelManager(ModelManager&&) = delete;

		ModelHandle LoadModel(const char* path);
		Model* GetModel(const ModelHandle& h);
		std::string GetModelPath(const ModelHandle& h);
		void ProcessLoadedModels();
		bool ShowGui(DebugGuiSystem& gui);
		void ReloadAll();

	private:
		struct ModelDesc 
		{
			std::unique_ptr<Model> m_model;
			std::string m_name;
		};
		struct ModelLoadResult
		{
			std::unique_ptr<Assets::Model> m_model;
			std::unique_ptr<Model> m_renderModel;
			ModelHandle m_destinationHandle;
		};
		std::unique_ptr<Render::Mesh> CreateMeshPart(const Assets::ModelMesh&);
		std::unique_ptr<Model> CreateModel(Assets::Model& model, std::vector<std::unique_ptr<Render::Mesh>>& meshes);
		void FinaliseModel(Assets::Model& model, Model& renderModel);

		std::vector<ModelDesc> m_models;
	
		Core::Mutex m_loadedModelsMutex;
		std::vector<ModelLoadResult> m_loadedModels;	// models to process after successful load
		std::atomic<int32_t> m_inFlightModels = 0;

		JobSystem* m_jobSystem;
	};
}