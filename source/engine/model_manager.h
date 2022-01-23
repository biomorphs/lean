#pragma once
#include "serialisation.h"
#include "system.h"
#include "model.h"
#include "model_asset.h"
#include "core/mutex.h"
#include "render/mesh_builder.h"
#include <string>
#include <vector>
#include <memory>

namespace Engine
{
	struct ModelHandle
	{
		SERIALISED_CLASS();
		uint32_t m_index = -1;
		static ModelHandle Invalid() { return { (uint32_t)-1 }; };
	};

	class ModelManager : public System
	{
	public:
		ModelHandle LoadModel(const char* path);
		Model* GetModel(const ModelHandle& h);
		std::string GetModelPath(const ModelHandle& h);
		void ReloadAll();

		virtual bool Tick(float timeDelta);
		virtual void Shutdown();

	private:
		void ProcessLoadedModels();
		bool ShowGui(DebugGuiSystem& gui);

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
	};
}