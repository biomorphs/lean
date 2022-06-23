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
		void SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader, const Render::Material* instanceMat);
		void SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader);
		void SetLight(glm::vec4 positionAndType, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation);
		void SetLight(glm::vec4 positionAndType, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation,
					  Render::FrameBuffer& sm, float shadowBias, glm::mat4 shadowMatrix, bool updateShadowmap);
		void SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, glm::vec2 spotAngles,
			Render::FrameBuffer& sm, float shadowBias, glm::mat4 shadowMatrix, bool updateShadowmap);
		void SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, glm::vec2 spotAngles);
		void SetClearColour(glm::vec4 c) { m_clearColour = c; }

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
		void SetUseOldCulling(bool t) { m_useOldCulling = t; }
		bool GetUseOldCulling() { return m_useOldCulling; }
		void SetCullingEnabled(bool b) { m_cullingEnabled = b; }
		void SetWireframeMode(bool m) { m_showWireframe = m; }
		bool GetWireframeMode() { return m_showWireframe; }
	private:
		using RenderInstance = MeshInstance<PerInstanceData>;
		struct InstanceList
		{
			std::vector<RenderInstance> m_instances;
		};
		using ShadowShaders = std::unordered_map<uint32_t, ShaderHandle>;

		uint32_t BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit);
		int RenderShadowmap(Render::Device& d, Light& l, const std::vector<std::unique_ptr<InstanceList>>& visibleInstances, const std::vector<size_t>& counts, int instanceListStartIndex);
		void RenderShadowmap(Render::Device& d, Light& l);
		void SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const Render::Mesh& mesh, const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat = nullptr);
		void SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns,
			const Render::VertexArray* va, const Render::RenderBuffer* ib, const Render::MeshChunk* chunks, uint32_t chunkCount, const Render::Material* meshMaterial,
			const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat = nullptr);
		void SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
			const struct ShaderHandle& shader, const Render::VertexArray* va, const Render::RenderBuffer* ib, 
			const Render::MeshChunk* chunks, uint32_t chunkCount, const PerInstanceData& pid);
		int PrepareOpaqueInstances(InstanceList& list);
		int PrepareTransparentInstances(InstanceList& list);
		int PrepareCulledShadowInstances(InstanceList& visibleInstances);
		int PrepareShadowInstances(glm::mat4 lightViewProj, InstanceList& visibleInstances);
		int PopulateInstanceBuffers(InstanceList& list, size_t instanceCount = -1);	// returns offset to start of index data in global gpu buffers
		void DrawInstances(Render::Device& d, const InstanceList& list, int baseIndex, bool bindShadowmaps=false, Render::UniformBuffer* uniforms = nullptr, size_t drawCount=-1);
		void UpdateGlobals(glm::mat4 projectionMat, glm::mat4 viewMat);
		void CullLights();

		// cull one source list into multiple result lists each with a different frustum
		void CullInstances(const InstanceList& srcInstances, InstanceList* results, const class Frustum* frustums, int listCount=1);

		using OnFindVisibleComplete = std::function<void(size_t)>;	// param = num. results found (note the result vector is NOT resized!)
		void FindVisibleInstancesAsync(const Frustum& f, const std::vector<RenderInstance>& src, std::vector<RenderInstance>& result, OnFindVisibleComplete onComplete);

		FrameStats m_frameStats;
		float m_hdrExposure = 1.0f;
		std::vector<Light> m_lights;
		InstanceList m_opaqueInstances;
		InstanceList m_transparentInstances;
		InstanceList m_allShadowCasterInstances;
		Render::RenderBuffer m_perInstanceData;	// global instance data
		int m_nextInstance = 0;				// index into buffers above
		bool m_cullingEnabled = true;
		bool m_useOldCulling = true;
		bool m_showWireframe = false;
		glm::vec4 m_clearColour = { 0.0f,0.0f,0.0f,1.0f };
		JobSystem* m_jobSystem = nullptr;
		float m_bloomThreshold = 1.0f;
		float m_bloomMultiplier = 0.5f;
		Render::RenderTargetBlitter m_targetBlitter;
		Engine::ShaderHandle m_blitShader;
		Engine::ShaderHandle m_tonemapShader;
		Engine::ShaderHandle m_bloomBrightnessShader;
		Engine::ShaderHandle m_bloomBlurShader;
		Engine::ShaderHandle m_bloomCombineShader;
		Render::RenderBuffer m_globalsUniformBuffer;
		Render::FrameBuffer m_mainFramebuffer;
		Render::FrameBuffer m_mainFramebufferResolved;
		Render::FrameBuffer m_bloomBrightnessBuffer;
		std::unique_ptr<Render::FrameBuffer> m_bloomBlurBuffers[2];
		Render::Camera m_camera;
		glm::ivec2 m_windowSize;
	};
}
