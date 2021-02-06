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

	class Renderer : public Render::RenderPass
	{
	public:
		Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, glm::ivec2 windowSize);			
		virtual ~Renderer() = default;

		void Reset();
		void RenderAll(Render::Device&);
		void SetCamera(const Render::Camera& c);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader);
		void SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model, const struct ShaderHandle& shader);
		void SetLight(glm::vec4 positionOrDir, glm::vec3 colour, float ambientStr, glm::vec3 attenuation);
		void SetLight(glm::vec4 positionOrDir, glm::vec3 colour, float ambientStr, glm::vec3 attenuation, Render::FrameBuffer& sm);
		void SetClearColour(glm::vec4 c) { m_clearColour = c; }
		void SetShadowsShader(ShaderHandle lightingShader, ShaderHandle shadowShader);

		struct FrameStats {
			size_t m_instancesSubmitted = 0;
			size_t m_shaderBinds = 0;
			size_t m_vertexArrayBinds = 0;
			size_t m_batchesDrawn = 0;
			size_t m_drawCalls = 0;
			size_t m_totalVertices = 0;
			size_t m_activeLights = 0;
		};
		const FrameStats& GetStats() const { return m_frameStats; }
		float& GetExposure() { return m_hdrExposure; }
		float& GetShadowBias() { return m_shadowBias; }
		float& GetCubeShadowBias() { return m_cubeShadowBias; }
	private:
		struct InstanceList
		{
			std::vector<MeshInstance> m_instances;
			Render::RenderBuffer m_transforms;
			Render::RenderBuffer m_colours;
		};
		using ShadowShaders = std::unordered_map<uint32_t, ShaderHandle>;

		uint32_t BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit);
		void RenderShadowmap(Render::Device& d, Light& l);
		void SubmitInstance(InstanceList& list, glm::vec3 cameraPos, glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader);
		void CreateInstanceList(InstanceList& newlist, uint32_t maxInstances);
		void PrepareOpaqueInstances(InstanceList& list);
		void PrepareTransparentInstances(InstanceList& list);
		void PrepareShadowInstances(glm::mat4 lightViewProj, InstanceList& visibleInstances);
		void PopulateInstanceBuffers(InstanceList& list);
		void DrawInstances(Render::Device& d, const InstanceList& list, bool bindShadowmaps=false, Render::UniformBuffer* uniforms = nullptr);
		void UpdateGlobals(glm::mat4 projectionMat, glm::mat4 viewMat);

		FrameStats m_frameStats;
		float m_hdrExposure = 1.0f;
		std::vector<Light> m_lights;
		InstanceList m_opaqueInstances;
		InstanceList m_transparentInstances;
		InstanceList m_allShadowCasterInstances;
		InstanceList m_visibleShadowInstances;
		glm::vec4 m_clearColour = { 0.0f,0.0f,0.0f,1.0f };
		float m_shadowBias = 0.002f;
		float m_cubeShadowBias = 0.5f;
		ShadowShaders m_shadowShaders;	// map of lighting shader handle index -> shadow shader
		ShaderManager* m_shaders;
		TextureManager* m_textures;
		ModelManager* m_models;
		Render::RenderTargetBlitter m_targetBlitter;
		Engine::ShaderHandle m_blitShader;
		Render::RenderBuffer m_globalsUniformBuffer;
		Render::FrameBuffer m_mainFramebuffer;
		Render::Camera m_camera;
		glm::ivec2 m_windowSize;
	};
}
