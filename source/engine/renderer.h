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
}

namespace Engine
{
	class TextureManager;
	class ModelManager;
	class JobSystem;

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, JobSystem* js, glm::ivec2 windowSize);
		virtual ~Renderer() = default;

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model, const struct ShaderHandle& shader);
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
		bool IsCullingEnabled() { return m_cullingEnabled; }
		void SetCullingEnabled(bool b) { m_cullingEnabled = b; }
	private:
		struct InstanceList
		{
			std::vector<MeshInstance> m_instances;
		};
		using ShadowShaders = std::unordered_map<uint32_t, ShaderHandle>;

		uint32_t BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit);
		void RenderShadowmap(Render::Device& d, Light& l);
		void SubmitInstance(InstanceList& list, glm::vec3 cameraPos, glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader);
		void SubmitInstance(InstanceList& list, glm::vec3 cam, glm::mat4 trns, glm::vec4 col, const Render::Mesh& mesh, const struct ShaderHandle& shader, glm::vec3 aabbMin, glm::vec3 aabbMax);
		int PrepareOpaqueInstances(InstanceList& list);
		int PrepareTransparentInstances(InstanceList& list);
		int PrepareCulledShadowInstances(InstanceList& visibleInstances);
		int PrepareShadowInstances(glm::mat4 lightViewProj, InstanceList& visibleInstances);
		int PopulateInstanceBuffers(InstanceList& list);	// returns offset to start of index data in global gpu buffers
		void DrawInstances(Render::Device& d, const InstanceList& list, int baseIndex, bool bindShadowmaps=false, Render::UniformBuffer* uniforms = nullptr);
		void UpdateGlobals(glm::mat4 projectionMat, glm::mat4 viewMat);
		void CullLights();

		// cull one source list into multiple result lists each with a different frustum
		void CullInstances(const InstanceList& srcInstances, InstanceList* results, const class Frustum* frustums, int listCount=1);

		FrameStats m_frameStats;
		float m_hdrExposure = 1.0f;
		std::vector<Light> m_lights;
		InstanceList m_opaqueInstances;
		InstanceList m_transparentInstances;
		InstanceList m_allShadowCasterInstances;
		Render::RenderBuffer m_transforms;	// global instance transforms
		Render::RenderBuffer m_colours;		// global instance colours
		int m_nextInstance = 0;				// index into buffers above
		bool m_cullingEnabled = true;
		glm::vec4 m_clearColour = { 0.0f,0.0f,0.0f,1.0f };
		ShaderManager* m_shaders;
		TextureManager* m_textures;
		ModelManager* m_models;
		JobSystem* m_jobSystem = nullptr;
		Render::RenderTargetBlitter m_targetBlitter;
		Engine::ShaderHandle m_blitShader;
		Render::RenderBuffer m_globalsUniformBuffer;
		Render::FrameBuffer m_mainFramebuffer;
		Render::Camera m_camera;
		glm::ivec2 m_windowSize;
	};
}
