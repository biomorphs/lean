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
		ModelHandle LoadModel(const char* path, std::function<void(bool, ModelHandle)> onFinish = nullptr);
		Model* GetModel(const ModelHandle& h);
		std::string GetModelPath(const ModelHandle& h);
		void ReloadAll();

		Render::VertexArray* GetVertexArray() { return m_globalVertexArray.get(); }
		Render::RenderBuffer* GetIndexBuffer() { return m_globalIndexData.get(); }

		virtual bool Initialise();
		virtual bool Tick(float timeDelta);
		virtual void Shutdown();

	private:
		void ProcessLoadedModels();
		bool ShowGui(DebugGuiSystem& gui);

		struct ModelDesc 
		{
			std::unique_ptr<Model> m_renderModel;
			std::string m_name;
		};
		struct ModelLoadResult
		{
			std::unique_ptr<Assets::Model> m_model;
			std::unique_ptr<Model> m_renderModel;
			ModelHandle m_destinationHandle;
			std::function<void(bool, ModelHandle)> m_onFinish;
		};
		std::unique_ptr<Model> CreateNewModel(const Assets::Model&);
		void FinaliseModel(Assets::Model& model, Model& renderModel);

		std::vector<ModelDesc> m_models;
	
		Core::Mutex m_loadedModelsMutex;
		std::vector<ModelLoadResult> m_loadedModels;	// models to process after successful load
		std::atomic<int32_t> m_inFlightModels = 0;

		// all models are loaded into these buffers
		uint32_t AllocateIndices(uint32_t count);
		uint32_t AllocateVertices(uint32_t count);
		std::unique_ptr<Render::VertexArray> m_globalVertexArray;
		std::unique_ptr<Render::RenderBuffer> m_globalVertexData;
		std::atomic<uint32_t> m_nextVertex = 0;
		std::unique_ptr<Render::RenderBuffer> m_globalIndexData;
		std::atomic<uint32_t> m_nextIndex = 0;
	};
}