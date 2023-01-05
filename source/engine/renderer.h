#pragma once
#include "render/render_pass.h"
#include "render/render_buffer.h"
#include "render/frame_buffer.h"
#include "render/camera.h"
#include "render/render_target_blitter.h"
#include "core/glm_headers.h"
#include "mesh_instance.h"
#include "light.h"
#include "shader_manager.h"
#include "model_manager.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Render
{
	class Material;
	class UniformBuffer;
	class VertexArray;
	class MeshChunk;
}

namespace Engine
{
	class JobSystem;
	class Frustum;

	const uint32_t c_maxLightsPerTile = 128;

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(JobSystem* js, glm::ivec2 windowSize);
		virtual ~Renderer() = default;

		struct PerInstanceData	
		{
			glm::vec4 m_diffuseOpacity;
			glm::vec4 m_specular;	//r,g,b,strength
			glm::vec4 m_shininess;	// add more stuff here!
			uint64_t m_diffuseTexture;
			uint64_t m_normalsTexture;
			uint64_t m_specularTexture;
		};

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);

		// Note the most optimal way to draw a LOT of different meshes is via PerInstanceData
		// Treat the per-mesh/per-instance materials as PUSH constants that force a pipeline switch!
		//	(i.e. they are really useful for general use, but use PerInstanceData if you generate a lot of material switches)
		void SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, const Render::Material* instanceMat = nullptr);
		void SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, glm::vec3 boundsMin, glm::vec3 boundsMax, const Render::Material* instanceMat = nullptr);
		void SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader);
		void SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader, const Model::MeshPart::DrawData* partOverride, uint32_t overrideCount);
		
		void DirectionalLight(glm::vec3 direction, glm::vec3 colour, float ambientStr, 
			Render::FrameBuffer* shadowMap=nullptr, float shadowBias = 0.0f, glm::mat4 shadowMatrix=glm::identity<glm::mat4>());
		void SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, glm::vec2 spotAngles,
			Render::FrameBuffer* shadowMap = nullptr, float shadowBias = 0.0f, glm::mat4 shadowMatrix = glm::identity<glm::mat4>());
		void PointLight(glm::vec3 position, glm::vec3 colour, float ambientStr, float distance, float attenuation, 
			Render::FrameBuffer* shadowMap = nullptr, float shadowBias = 0.0f, glm::mat4 shadowMatrix = glm::identity<glm::mat4>());

		void SetClearColour(glm::vec4 c) { m_clearColour = c; }

		void ForEachUsedRT(std::function<void(const char*, Render::FrameBuffer&)> rtFn);
		void DrawLightTilesDebug(class RenderContext2D& r2d);

		struct FrameStats {
			size_t m_instancesSubmitted = 0;
			size_t m_totalTransparentInstances = 0;
			size_t m_totalOpaqueInstances = 0;
			size_t m_totalShadowInstances = 0;
			size_t m_renderedTransparentInstances = 0;
			size_t m_renderedOpaqueInstances = 0;
			size_t m_renderedShadowInstances = 0;
			size_t m_shadowMapUpdates = 0;
			size_t m_shaderBinds = 0;
			size_t m_vertexArrayBinds = 0;
			size_t m_batchesDrawn = 0;
			size_t m_drawCalls = 0;
			size_t m_totalVertices = 0;
			size_t m_activeLights = 0;
			size_t m_visibleLights = 0;
		};
		const FrameStats& GetStats() const { return m_frameStats; }
		float GetExposure() { return m_hdrExposure; }
		void SetExposure(float e) { m_hdrExposure = e; }
		float GetBloomThreshold() { return m_bloomThreshold; }
		void SetBloomThreshold(float t) { m_bloomThreshold = t; }
		float GetBloomMultiplier() { return m_bloomMultiplier; }
		void SetBloomMultiplier(float t) { m_bloomMultiplier = t; }
		bool IsCullingEnabled() { return m_cullingEnabled; }
		void SetUseDrawIndirect(bool t) { m_useDrawIndirect = t; }
		bool GetUseDrawIndirect() { return m_useDrawIndirect; }
		void SetCullingEnabled(bool b) { m_cullingEnabled = b; }
		void SetWireframeMode(bool m) { m_showWireframe = m; }
		bool GetWireframeMode() { return m_showWireframe; }
	private:
		struct InstanceList
		{
			struct TransformBounds {
				glm::mat4 m_transform;
				glm::vec3 m_aabbMin;
				glm::vec3 m_aabbMax;
			};
			struct DrawData {
				Render::ShaderProgram* m_shader;
				const Render::VertexArray* m_va;
				const Render::RenderBuffer* m_ib;
				const Render::Material* m_meshMaterial;
				const Render::Material* m_instanceMaterial;
				const Render::MeshChunk* m_chunks;
				uint32_t m_chunkCount;
			};
			struct Entry {
				__m128i m_sortKey;
				uint64_t m_dataIndex;
				uint64_t m_padding;
			};
			std::vector<TransformBounds> m_transformBounds;
			std::vector<DrawData> m_drawData;
			std::vector<PerInstanceData> m_perInstanceData;
			std::vector<Entry> m_entries;
			
			void Reserve(size_t count);
			void Resize(size_t count);
			void Reset();
		};
		using EntryList = std::vector<InstanceList::Entry>;
		using ShadowShaders = std::unordered_map<uint32_t, ShaderHandle>;

		struct DrawBucket
		{
			Render::ShaderProgram* m_shader;
			const Render::VertexArray* m_va;
			const Render::RenderBuffer* m_ib;
			const Render::Material* m_meshMaterial;
			const Render::Material* m_instanceMaterial;
			uint32_t m_primitiveType;	// Render::PrimitiveType
			int m_firstDrawIndex;
			int m_drawCount;
		};

		uint32_t BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit);
		int RenderShadowmap(Render::Device& d, Light& l, const std::vector<std::unique_ptr<EntryList>>& visibleInstances, int instanceListStartIndex);

		void SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const Render::Mesh& mesh, const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat = nullptr);
		void SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns,
			const Render::VertexArray* va, const Render::RenderBuffer* ib, const Render::MeshChunk* chunks, uint32_t chunkCount, const Render::Material* meshMaterial,
			const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat = nullptr);
		void SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
			const struct ShaderHandle& shader, const Render::VertexArray* va, const Render::RenderBuffer* ib, 
			const Render::MeshChunk* chunks, uint32_t chunkCount, const PerInstanceData& pid);

		int PopulateInstanceBuffers(InstanceList& list, const EntryList& entries);	// returns offset to start of index data in global gpu buffers
		void PrepareDrawBuckets(const InstanceList& list, const EntryList& entries, int baseIndex, std::vector<DrawBucket>& buckets);
		void DrawBuckets(Render::Device& d, const std::vector<DrawBucket>& buckets, bool bindShadowmaps, Render::UniformBuffer* uniforms);
		void DrawInstances(Render::Device& d, const InstanceList& list, const EntryList& entries, int baseIndex, bool bindShadowmaps = false, Render::UniformBuffer* uniforms = nullptr);

		void UpdateGlobals(glm::mat4 projectionMat, glm::mat4 viewMat);
		void CullLights();
		void ClassifyLightTiles();

		using OnFindVisibleComplete = std::function<void(size_t)>;	// param = num. results found (note the result vector is NOT resized!)
		void FindVisibleInstancesAsync(const Frustum& f, const InstanceList& src, EntryList& result, OnFindVisibleComplete onComplete);

		FrameStats m_frameStats;
		class ModelManager* m_modelManager = nullptr;
		class ShaderManager* m_shaderManager = nullptr;
		class TextureManager* m_textureManager = nullptr;
		uint64_t m_defaultDiffuseResidentHandle = -1;
		uint64_t m_defaultNormalResidentHandle = -1;
		uint64_t m_defaultSpecularResidentHandle = -1;
		float m_hdrExposure = 1.0f;
		std::vector<Light> m_lights;
		InstanceList m_opaqueInstances;
		InstanceList m_transparentInstances;
		InstanceList m_allShadowCasterInstances;
		Render::RenderBuffer m_perInstanceData;	// global instance data
		Render::RenderBuffer m_drawIndirectBuffer;
		int m_nextInstance = 0;				// index into buffers above
		int m_nextDrawCall = 0;
		bool m_cullingEnabled = true;
		bool m_useDrawIndirect = false;
		bool m_showWireframe = false;
		glm::vec4 m_clearColour = { 0.0f,0.0f,0.0f,1.0f };
		JobSystem* m_jobSystem = nullptr;
		float m_bloomThreshold = 1.0f;
		float m_bloomMultiplier = 0.5f;
		struct LightTileInfo
		{
			uint32_t m_currentCount;
			uint32_t m_lightIndices[c_maxLightsPerTile];
		};
		glm::ivec2 m_lightTileCounts = { 32, 16 };
		std::vector<LightTileInfo> m_lightTiles;
		Render::RenderBuffer m_allLightsData;
		Render::RenderBuffer m_lightTileData;
		Render::RenderTargetBlitter m_targetBlitter;
		Engine::ShaderHandle m_blitShader;
		Engine::ShaderHandle m_tonemapShader;
		Engine::ShaderHandle m_bloomBrightnessShader;
		Engine::ShaderHandle m_bloomBlurShader;
		Engine::ShaderHandle m_bloomCombineShader;
		Render::RenderBuffer m_globalsUniformBuffer;
		Render::FrameBuffer m_mainFramebuffer;
		Render::FrameBuffer m_mainFramebufferResolved;
		Render::FrameBuffer m_mainDepthResolved;
		Render::FrameBuffer m_bloomBrightnessBuffer;
		std::unique_ptr<Render::FrameBuffer> m_bloomBlurBuffers[2];
		Render::Camera m_camera;
		glm::ivec2 m_windowSize;
	};
}
