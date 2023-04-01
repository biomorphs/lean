#pragma once
#include "render/render_pass.h"
#include "render/render_buffer.h"
#include "render/frame_buffer.h"
#include "render/camera.h"
#include "render/render_target_blitter.h"
#include "core/glm_headers.h"
#include "core/mutex.h"
#include "light.h"
#include "shader_manager.h"
#include "model_manager.h"
#include "render_instance_list.h"
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
	class RenderInstances;
	class SSAO;
	const uint32_t c_maxLightsPerTile = 256;
	const uint32_t c_maxFramesAhead = 3;
	const int c_msaaSamples = 1;

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(JobSystem* js, glm::ivec2 windowSize);
		virtual ~Renderer() = default;

		uint64_t GetDefaultDiffuseTexture() const { return m_defaultDiffuseResidentHandle; }
		uint64_t GetDefaultSpecularTexture() const { return m_defaultSpecularResidentHandle; }
		uint64_t GetDefaultNormalsTexture() const { return m_defaultNormalResidentHandle; }
		SSAO& GetSSAO() { return *m_ssao; }

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);

		// Note the most optimal way to draw a LOT of different meshes is via PerInstanceData
		// Treat the per-mesh/per-instance materials as PUSH constants that force a pipeline switch!
		//	(i.e. they are really useful for general use, but use PerInstanceData if you generate a lot of material switches)
		void SubmitInstances(const __m128* positions, int count, const struct ModelHandle& model, const struct ShaderHandle& shader);
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
		void SetDeferredRenderEnabled(bool m) { m_drawDeferred = m; }
		bool GetDeferredRenderEnabled() const { return m_drawDeferred; }
	private:
		using EntryList = std::vector<RenderInstanceList::Entry>;
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

		int PopulateInstanceBuffers(RenderInstanceList& list, const EntryList& entries);	// returns offset to start of index data in global gpu buffers
		void PrepareDrawBuckets(const RenderInstanceList& list, const EntryList& entries, int baseIndex, std::vector<DrawBucket>& buckets);
		void DrawBuckets(Render::Device& d, const std::vector<DrawBucket>& buckets, bool bindShadowmaps, Render::UniformBuffer* uniforms);
		void DrawInstances(Render::Device& d, const RenderInstanceList& list, const EntryList& entries, int baseIndex, bool bindShadowmaps = false, Render::UniformBuffer* uniforms = nullptr);

		void UpdateGlobals(glm::mat4 projectionMat, glm::mat4 viewMat);
		void CullLights();
		void ClassifyLightTiles();
		void UploadLightTiles();
		void BeginCullingAsync();
		void RenderShadowMaps(Render::Device& d);
		void RenderOpaquesDeferred(Render::Device& d);
		void RenderOpaquesForward(Render::Device& d);
		void RenderTransparents(Render::Device& d);
		void RenderPostFx(Render::Device& d, Render::FrameBuffer& src);

		using OnFindVisibleComplete = std::function<void(size_t)>;	// param = num. results found (note the result vector is NOT resized!)
		void FindVisibleInstancesAsync(const Frustum& f, const RenderInstanceList& src, EntryList& result, OnFindVisibleComplete onComplete);

		// culling 
		EntryList m_visibleOpaquesFwd;
		EntryList m_visibleOpaquesDeferred;
		EntryList m_visibleTransparents;
		std::vector<std::unique_ptr<EntryList>> m_visibleShadowCasters;
		std::atomic<bool> m_opaquesFwdReady = false;
		std::atomic<bool> m_opaquesDeferredReady = false;
		std::atomic<bool> m_transparentsReady = false;
		std::atomic<int> m_shadowsInFlight = 0;

		FrameStats m_frameStats;
		class ModelManager* m_modelManager = nullptr;
		class ShaderManager* m_shaderManager = nullptr;
		class TextureManager* m_textureManager = nullptr;
		uint64_t m_defaultDiffuseResidentHandle = -1;
		uint64_t m_defaultNormalResidentHandle = -1;
		uint64_t m_defaultSpecularResidentHandle = -1;
		float m_hdrExposure = 1.0f;
		Core::Mutex m_addLightMutex;		// note adding lights is not safe once render has started!
		std::vector<Light> m_lights;
		RenderInstances m_allInstances;
		int m_nextInstance = 0;				// index into buffers above
		int m_nextDrawCall = 0;
		bool m_cullingEnabled = true;
		bool m_useDrawIndirect = false;
		bool m_showWireframe = false;
		bool m_drawDeferred = true;
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
		Render::RenderBuffer m_allLightsData[c_maxFramesAhead];
		Render::RenderBuffer m_lightTileData[c_maxFramesAhead];
		Render::RenderBuffer m_perInstanceData[c_maxFramesAhead];	// global instance data
		Render::RenderBuffer m_drawIndirectBuffer[c_maxFramesAhead];
		Render::RenderBuffer m_globalsUniformBuffer[c_maxFramesAhead];
		Render::RenderTargetBlitter m_targetBlitter;
		Engine::ShaderHandle m_blitShader;
		Engine::ShaderHandle m_tonemapShader;
		Engine::ShaderHandle m_bloomBrightnessShader;
		Engine::ShaderHandle m_bloomBlurShader;
		Engine::ShaderHandle m_bloomCombineShader;
		Engine::ShaderHandle m_deferredLightingShader;
		Render::FrameBuffer m_mainFramebuffer;
		Render::FrameBuffer m_gBuffer;
		Render::FrameBuffer m_mainFramebufferResolved;
		Render::FrameBuffer m_bloomBrightnessBuffer;
		std::unique_ptr<Render::FrameBuffer> m_bloomBlurBuffers[2];
		std::unique_ptr<SSAO> m_ssao;
		int m_currentBuffer = 0;	// max = c_maxFramesAhead
		Render::Camera m_camera;
		glm::ivec2 m_windowSize;
	};
}
