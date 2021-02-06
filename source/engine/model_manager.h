#pragma once
#include "model.h"
#include "model_asset.h"
#include "render/mesh_builder.h"
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
	class TextureManager;
	class DebugGuiSystem;
	class JobSystem;

	struct ModelHandle
	{
		uint16_t m_index = -1;
		static ModelHandle Invalid() { return { (uint16_t)-1 }; };
	};

	class ModelManager
	{
	public:
		ModelManager(TextureManager* tm, JobSystem* js);
		~ModelManager() = default;
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
			std::vector<std::unique_ptr<Render::MeshBuilder>> m_meshBuilders;
			ModelHandle m_destinationHandle;
		};
		std::unique_ptr<Render::MeshBuilder> CreateBuilderForPart(const Assets::ModelMesh&);
		std::unique_ptr<Model> CreateModel(Assets::Model& model, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders);
		void FinaliseModel(Assets::Model& model, Model& renderModel, const std::vector<std::unique_ptr<Render::MeshBuilder>>& meshBuilders);

		std::vector<ModelDesc> m_models;
	
		std::mutex m_loadedModelsMutex;
		std::vector<ModelLoadResult> m_loadedModels;	// models to process after successful load
		std::atomic<int32_t> m_inFlightModels = 0;

		TextureManager* m_textureManager;
		JobSystem* m_jobSystem;
	};
}