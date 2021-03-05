#include "renderer.h"
#include "core/log.h"
#include "core/profiler.h"
#include "core/string_hashing.h"
#include "core/thread.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/material.h"
#include "frustum.h"
#include "mesh_instance.h"
#include "model_manager.h"
#include "shader_manager.h"
#include "model.h"
#include "material_helpers.h"
#include "job_system.h"
#include <algorithm>
#include <map>

namespace Engine
{
	const uint64_t c_maxInstances = 1024 * 512;		// this needs to include all passes/shadowmap updates
	const uint64_t c_maxLights = 64;
	const uint32_t c_maxShadowMaps = 16;

	struct LightInfo					// passed to shaders
	{
		glm::vec4 m_colourAndAmbient;	// rgb=colour, a=ambient multiplier
		glm::vec4 m_position;			// w = type (0=Directional,1=Point,2=Spot)
		glm::vec3 m_direction;	
		glm::vec2 m_distanceAttenuation;// distance, attenuation compression
		glm::vec2 m_spotAngles;			// spotlight angles (inner,outer)
		glm::vec3 m_shadowParams;		// enabled, index, bias
		glm::mat4 m_lightSpaceMatrix;	// for shadow calculations
	};

	struct GlobalUniforms				// passed to shaders
	{
		glm::mat4 m_viewProjMat;
		glm::vec4 m_cameraPosition;		// world-space
		LightInfo m_lights[c_maxLights];
		int m_lightCount;
		float m_hdrExposure;
	};

	std::map<std::string, TextureHandle> g_defaultTextures;

	Renderer::Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, JobSystem* js, glm::ivec2 windowSize)
		: m_textures(ta)
		, m_models(mm)
		, m_shaders(sm)
		, m_jobSystem(js)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
	{
		g_defaultTextures["DiffuseTexture"] = m_textures->LoadTexture("white.bmp");
		g_defaultTextures["NormalsTexture"] = m_textures->LoadTexture("default_normalmap.png");
		g_defaultTextures["SpecularTexture"] = m_textures->LoadTexture("white.bmp");
		m_blitShader = m_shaders->LoadShader("Basic Blit", "basic_blit.vs", "basic_blit.fs");
		{
			SDE_PROF_EVENT("Create Buffers");
			m_transforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
			m_colours.Create(c_maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Dynamic, true);
			m_opaqueInstances.m_instances.reserve(c_maxInstances);
			m_transparentInstances.m_instances.reserve(c_maxInstances);
			m_allShadowCasterInstances.m_instances.reserve(c_maxInstances);
		}
		{
			SDE_PROF_EVENT("Create render targets");
			m_mainFramebuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F16);
			m_mainFramebuffer.AddDepthStencil();
			if (!m_mainFramebuffer.Create())
			{
				SDE_LOG("Failed to create framebuffer!");
			}
		}
	}

	void Renderer::SetShadowsShader(ShaderHandle lightingShader, ShaderHandle shadowShader)
	{
		m_shadowShaders[lightingShader.m_index] = shadowShader;
	}

	void Renderer::Reset() 
	{ 
		m_opaqueInstances.m_instances.clear();
		m_transparentInstances.m_instances.clear();
		m_allShadowCasterInstances.m_instances.clear();
		m_lights.clear();
		m_nextInstance = 0;
	}

	void Renderer::SetCamera(const Render::Camera& c)
	{ 
		m_camera = c;
	}

	bool IsMeshTransparent(const Render::Mesh& mesh, TextureManager& tm)
	{
		const uint32_t c_diffuseSampler = Core::StringHashing::GetHash("DiffuseTexture");
		const auto& samplers = mesh.GetMaterial().GetSamplers();
		const auto& diffuseSampler = samplers.find(c_diffuseSampler);
		if (diffuseSampler != samplers.end())
		{
			TextureHandle texHandle = { static_cast<uint16_t>(diffuseSampler->second.m_handle) };
			const auto theTexture = tm.GetTexture({ texHandle });
			if (theTexture && theTexture->GetComponentCount() == 4)
			{
				return true;
			}
		}
		return false;
	}

	void Renderer::SubmitInstance(InstanceList& list, glm::vec3 cam, glm::mat4 trns, glm::vec4 col, const Render::Mesh& mesh, const struct ShaderHandle& shader, glm::vec3 aabbMin, glm::vec3 aabbMax)
	{
		float distanceToCamera = glm::length(glm::vec3(trns[3]) - cam);
		const auto foundShader = m_shaders->GetShader(shader);
		list.m_instances.push_back({ trns, col, aabbMin, aabbMax, foundShader, &mesh, distanceToCamera });
	}

	void Renderer::SubmitInstance(InstanceList& list, glm::vec3 cameraPos, glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader)
	{
		// objects submitted with no bounds have infinite aabb
		const auto boundsMin = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		const auto boundsMax = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		SubmitInstance(list, cameraPos, transform, colour, mesh, shader, boundsMin, boundsMax);
	}

	void Renderer::SubmitInstance(glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader)
	{
		bool castShadow = true;
		if (castShadow)
		{
			const auto& foundShadowShader = m_shadowShaders.find(shader.m_index);
			if (foundShadowShader != m_shadowShaders.end())
			{
				SubmitInstance(m_allShadowCasterInstances, m_camera.Position(), transform, colour, mesh, foundShadowShader->second);
			}
		}

		bool isTransparent = colour.a != 1.0f;
		if (!isTransparent)
		{
			isTransparent = IsMeshTransparent(mesh, *m_textures);
		}
		InstanceList& instances = isTransparent ? m_transparentInstances : m_opaqueInstances;
		SubmitInstance(instances, m_camera.Position(), transform, colour, mesh, shader);
	}

	void Renderer::SubmitInstance(glm::mat4 transform, glm::vec4 colour, const struct ModelHandle& model, const struct ShaderHandle& shader)
	{
		const auto theModel = m_models->GetModel(model);
		const auto theShader = m_shaders->GetShader(shader);
		ShaderHandle shadowShader = ShaderHandle::Invalid();

		bool castShadow = true;
		if (castShadow)
		{
			const auto& foundShadowShader = m_shadowShaders.find(shader.m_index);
			if (foundShadowShader != m_shadowShaders.end())
			{
				shadowShader = foundShadowShader->second;
			}
		}

		if (theModel != nullptr && theShader != nullptr)
		{
			uint16_t meshPartIndex = 0;
			for (const auto& part : theModel->Parts())
			{
				const glm::mat4 instanceTransform = transform * part.m_transform;
				glm::vec3 boundsMin = part.m_boundsMin, boundsMax = part.m_boundsMax;
				if (shadowShader.m_index != -1)
				{
					SubmitInstance(m_allShadowCasterInstances, m_camera.Position(), transform, colour, *part.m_mesh, shadowShader, boundsMin, boundsMax);
				}

				bool isTransparent = colour.a != 1.0f;
				if (!isTransparent)
				{
					isTransparent = IsMeshTransparent(*part.m_mesh, *m_textures);
				}
				InstanceList& instances = isTransparent ? m_transparentInstances : m_opaqueInstances;
				SubmitInstance(instances, m_camera.Position(), instanceTransform, colour, *part.m_mesh, shader, boundsMin, boundsMax);
			}
		}
	}

	void Renderer::SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, glm::vec2 spotAngles)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = { position, 2.0f };	// spotlight
		newLight.m_direction = direction;
		newLight.m_maxDistance = distance;
		newLight.m_attenuationCompress = attenuation;
		newLight.m_spotlightAngles = spotAngles;
		m_lights.push_back(newLight);
	}

	void Renderer::SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, glm::vec2 spotAngles,
		Render::FrameBuffer& sm, float shadowBias, glm::mat4 shadowMatrix, bool updateShadowmap)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = { position, 2.0f };	// spotlight
		newLight.m_direction = direction;
		newLight.m_maxDistance = distance;
		newLight.m_attenuationCompress = attenuation;
		newLight.m_shadowMap = &sm;
		newLight.m_lightspaceMatrix = shadowMatrix;
		newLight.m_shadowBias = shadowBias;
		newLight.m_updateShadowmap = updateShadowmap;
		newLight.m_spotlightAngles = spotAngles;
		m_lights.push_back(newLight);
	}

	void Renderer::SetLight(glm::vec4 posAndType, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, 
		Render::FrameBuffer& sm, float bias, glm::mat4 shadowMatrix, bool updateShadowmap)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = posAndType;
		newLight.m_direction = direction;
		newLight.m_maxDistance = distance;
		newLight.m_attenuationCompress = attenuation;
		newLight.m_shadowMap = &sm;
		newLight.m_lightspaceMatrix = shadowMatrix;
		newLight.m_shadowBias = bias;
		newLight.m_updateShadowmap = updateShadowmap;
		m_lights.push_back(newLight);
	}

	void Renderer::SetLight(glm::vec4 posAndType, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = posAndType;
		newLight.m_direction = direction;
		newLight.m_maxDistance = distance;
		newLight.m_attenuationCompress = attenuation;
		m_lights.push_back(newLight);
	}

	int Renderer::PopulateInstanceBuffers(InstanceList& list)
	{
		SDE_PROF_EVENT();
		//static to avoid constant allocations
		static std::vector<glm::mat4> instanceTransforms;
		instanceTransforms.reserve(list.m_instances.size());
		instanceTransforms.clear();
		static std::vector<glm::vec4> instanceColours;
		instanceColours.reserve(list.m_instances.size());
		instanceColours.clear();
		for (const auto& c : list.m_instances)
		{
			instanceTransforms.push_back(c.m_transform);
			instanceColours.push_back(c.m_colour);
		}

		// copy the instance buffers to gpu
		int instanceIndex = -1;
		if (list.m_instances.size() > 0 && m_nextInstance + list.m_instances.size() < c_maxInstances)
		{
			m_transforms.SetData(m_nextInstance * sizeof(glm::mat4), instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
			m_colours.SetData(m_nextInstance * sizeof(glm::vec4), instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
			instanceIndex = m_nextInstance;
			m_nextInstance += list.m_instances.size();
		}
		return instanceIndex;
	}

	void Renderer::CullLights()
	{
		SDE_PROF_EVENT();
		// if a light is not visible to the main camera we can discard it
		const auto projectionMat = m_camera.ProjectionMatrix();
		const auto viewMat = m_camera.ViewMatrix();
		const Frustum f(projectionMat * viewMat);
		m_lights.erase(std::remove_if(m_lights.begin(), m_lights.end(), [&f](const Light& l) {
			if (l.m_position.w == 0.0f)			// directional lights always visible
				return false;
			else if (l.m_position.w == 1.0f)	// point light
			{
				return !f.IsSphereVisible(glm::vec3(l.m_position), l.m_maxDistance);
			}
			else if (l.m_position.w == 2.0f)		// spot light
			{
				Frustum lightFrustum(l.m_lightspaceMatrix);
				return !f.IsFrustumVisible(lightFrustum);
			}
			return false;
		}), m_lights.end());
		m_frameStats.m_visibleLights = m_lights.size();
	}

	void Renderer::UpdateGlobals(glm::mat4 projectionMat, glm::mat4 viewMat)
	{
		SDE_PROF_EVENT();
		GlobalUniforms globals;
		globals.m_viewProjMat = projectionMat * viewMat;
		uint32_t shadowMapIndex = 0;		// precalculate shadow map sampler indices
		uint32_t cubeShadowMapIndex = 0;
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			globals.m_lights[l].m_colourAndAmbient = m_lights[l].m_colour;
			globals.m_lights[l].m_position = m_lights[l].m_position;
			globals.m_lights[l].m_direction = m_lights[l].m_direction;
			globals.m_lights[l].m_distanceAttenuation = { m_lights[l].m_maxDistance, m_lights[l].m_attenuationCompress };
			globals.m_lights[l].m_spotAngles = m_lights[l].m_spotlightAngles;
			bool canAddMore = m_lights[l].m_position.w == 1.0f ? cubeShadowMapIndex < c_maxShadowMaps : shadowMapIndex < c_maxShadowMaps;
			if (m_lights[l].m_shadowMap && canAddMore)
			{
				globals.m_lights[l].m_shadowParams.x = 1.0f;
				globals.m_lights[l].m_shadowParams.y = m_lights[l].m_position.w == 1.0f ? cubeShadowMapIndex++ : shadowMapIndex++;
				globals.m_lights[l].m_shadowParams.z = m_lights[l].m_shadowBias;
				globals.m_lights[l].m_lightSpaceMatrix = m_lights[l].m_lightspaceMatrix;
			}
			else
			{
				globals.m_lights[l].m_shadowParams = { 0.0f,0.0f,0.0f };
			}
		}
		globals.m_lightCount = static_cast<int>(std::min(m_lights.size(), c_maxLights));
		globals.m_cameraPosition = glm::vec4(m_camera.Position(), 0.0);
		globals.m_hdrExposure = m_hdrExposure;
		m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);
	}

	uint32_t Renderer::BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit)
	{
		SDE_PROF_EVENT();
		uint32_t shadowMapIndex = 0;
		uint32_t cubeShadowMapIndex = 0;
		char samplerNameBuffer[256] = { '\0' };
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr)
			{
				if (m_lights[l].m_position.w == 0.0f || m_lights[l].m_position.w == 2.0f && shadowMapIndex < c_maxShadowMaps)
				{
					sprintf_s(samplerNameBuffer, "ShadowMaps[%d]", shadowMapIndex++);
					uint32_t uniformHandle = shader.GetUniformHandle(samplerNameBuffer);
					if (uniformHandle != -1)
					{
						d.SetSampler(uniformHandle, m_lights[l].m_shadowMap->GetDepthStencil()->GetHandle(), textureUnit++);
					}
				}
				else if(cubeShadowMapIndex < c_maxShadowMaps)
				{
					sprintf_s(samplerNameBuffer, "ShadowCubeMaps[%d]", cubeShadowMapIndex++);
					uint32_t uniformHandle = shader.GetUniformHandle(samplerNameBuffer);
					if (uniformHandle != -1)
					{
						d.SetSampler(uniformHandle, m_lights[l].m_shadowMap->GetDepthStencil()->GetHandle(), textureUnit++);
					}
				}
			}
		}
		// make sure to reset any we are not using or opengl will NOT be happy
		for (int l = shadowMapIndex; l < c_maxShadowMaps; ++l)
		{
			sprintf_s(samplerNameBuffer, "ShadowMaps[%d]", shadowMapIndex++);
			uint32_t uniformHandle = shader.GetUniformHandle(samplerNameBuffer);
			if (uniformHandle != -1)
			{
				d.SetSampler(uniformHandle, 0, textureUnit++);
			}
		}
		for (int l = cubeShadowMapIndex; l < c_maxShadowMaps; ++l)
		{
			sprintf_s(samplerNameBuffer, "ShadowCubeMaps[%d]", cubeShadowMapIndex++);
			uint32_t uniformHandle = shader.GetUniformHandle(samplerNameBuffer);
			if (uniformHandle != -1)
			{
				d.SetSampler(uniformHandle, 0, textureUnit++);
			}
		}

		return textureUnit;
	}

	void Renderer::DrawInstances(Render::Device& d, const InstanceList& list, int baseIndex, bool bindShadowmaps, Render::UniformBuffer* uniforms)
	{
		SDE_PROF_EVENT();
		auto firstInstance = list.m_instances.begin();
		const Render::ShaderProgram* lastShaderUsed = nullptr;	// avoid setting the same shader
		while (firstInstance != list.m_instances.end())
		{
			// Batch by shader and mesh
			auto lastMeshInstance = std::find_if(firstInstance, list.m_instances.end(), [firstInstance](const MeshInstance& m) -> bool {
				return  m.m_mesh != firstInstance->m_mesh || m.m_shader != firstInstance->m_shader;
				});
			auto instanceCount = (uint32_t)(lastMeshInstance - firstInstance);
			const Render::Mesh* theMesh = firstInstance->m_mesh;
			Render::ShaderProgram* theShader = firstInstance->m_shader;
			if (theShader != nullptr && theMesh != nullptr)
			{
				m_frameStats.m_batchesDrawn++;

				// bind shader + globals UBO
				if (theShader != lastShaderUsed)
				{
					m_frameStats.m_shaderBinds++;
					d.BindShaderProgram(*theShader);
					d.BindUniformBufferIndex(*theShader, "Globals", 0);
					d.SetUniforms(*theShader, m_globalsUniformBuffer, 0);
					if (uniforms != nullptr)
					{
						uniforms->Apply(d, *theShader);
					}
					lastShaderUsed = theShader;
				}

				// apply mesh material uniforms and samplers
				uint32_t textureUnit = 0;
				textureUnit = ApplyMaterial(d, *theShader, theMesh->GetMaterial(), *m_textures, g_defaultTextures, textureUnit);
				if (bindShadowmaps)
				{
					textureUnit = BindShadowmaps(d, *theShader, textureUnit+1);
				}

				// bind vertex array + instancing streams immediately after mesh vertex streams
				m_frameStats.m_vertexArrayBinds++;
				int instancingSlotIndex = theMesh->GetVertexArray().GetStreamCount();
				d.BindVertexArray(theMesh->GetVertexArray());
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_transforms, instancingSlotIndex++, 4, 0, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_transforms, instancingSlotIndex++, 4, sizeof(float) * 4, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_transforms, instancingSlotIndex++, 4, sizeof(float) * 8, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_transforms, instancingSlotIndex++, 4, sizeof(float) * 12, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), m_colours, instancingSlotIndex++, 4, 0);

				// draw the chunks
				for (const auto& chunk : theMesh->GetChunks())
				{
					uint32_t firstIndex = (uint32_t)(firstInstance - list.m_instances.begin());
					d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex + baseIndex);
					m_frameStats.m_drawCalls++;
					m_frameStats.m_totalVertices += (uint64_t)chunk.m_vertexCount * instanceCount;
				}
			}
			firstInstance = lastMeshInstance;
		}
	}

	void Renderer::RenderShadowmap(Render::Device& d, Light& l)
	{
		SDE_PROF_EVENT();
		m_frameStats.m_shadowMapUpdates++;
		if (l.m_position.w == 0.0f || l.m_position.w == 2.0f)		// directional / spot
		{
			InstanceList visibleShadowInstances;
			int baseIndex = PrepareShadowInstances(l.m_lightspaceMatrix, visibleShadowInstances);
			Render::UniformBuffer lightMatUniforms;
			lightMatUniforms.SetValue("ShadowLightSpaceMatrix", l.m_lightspaceMatrix);
			lightMatUniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
			d.DrawToFramebuffer(*l.m_shadowMap);
			d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
			d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);
			d.SetScissorEnabled(false);
			DrawInstances(d, visibleShadowInstances, baseIndex, false, &lightMatUniforms);
			m_frameStats.m_totalShadowInstances += m_allShadowCasterInstances.m_instances.size();
			m_frameStats.m_renderedShadowInstances += visibleShadowInstances.m_instances.size();
		}
		else
		{
			Render::UniformBuffer uniforms;
			const auto pos3 = glm::vec3(l.m_position);
			const glm::mat4 shadowTransforms[] = {
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0,0.0f))
			};
			InstanceList culledShadowInstances[6];	// we will cull lists for all 6 faces at once
			Frustum shadowFrustums[6];
			if (m_cullingEnabled)
			{
				for (uint32_t cubeFace = 0; cubeFace < 6; ++cubeFace)
				{
					shadowFrustums[cubeFace] = Frustum(shadowTransforms[cubeFace]);
					culledShadowInstances[cubeFace].m_instances.reserve(m_allShadowCasterInstances.m_instances.size() / 4);	// tune
				}
				CullInstances(m_allShadowCasterInstances, culledShadowInstances, shadowFrustums, 6);
			}
			for (uint32_t cubeFace = 0; cubeFace < 6; ++cubeFace)
			{
				auto& instances = m_cullingEnabled ? culledShadowInstances[cubeFace] : m_allShadowCasterInstances;
				int baseIndex = PrepareCulledShadowInstances(instances);
				d.DrawToFramebuffer(*l.m_shadowMap, cubeFace);
				d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
				d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
				d.SetBackfaceCulling(true, true);	// backface culling, ccw order
				d.SetBlending(false);
				d.SetScissorEnabled(false);
				uniforms.SetValue("ShadowLightSpaceMatrix", shadowTransforms[cubeFace]);
				uniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
				DrawInstances(d, instances, baseIndex, false, &uniforms);
				m_frameStats.m_totalShadowInstances += m_allShadowCasterInstances.m_instances.size();
				m_frameStats.m_renderedShadowInstances += instances.m_instances.size();
			}
		}
	}

	void Renderer::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();
		auto totalInstances = m_opaqueInstances.m_instances.size() + m_transparentInstances.m_instances.size();
		m_frameStats = {};
		m_frameStats.m_instancesSubmitted = totalInstances;
		m_frameStats.m_activeLights = std::min(m_lights.size(), c_maxLights);

		{
			SDE_PROF_EVENT("Clear main framebuffer");
			d.SetDepthState(true, true);	// make sure depth write is enabled before clearing!
			d.ClearFramebufferColourDepth(m_mainFramebuffer, m_clearColour, FLT_MAX);
		}

		CullLights();

		// setup global constants
		const auto projectionMat = m_camera.ProjectionMatrix();
		const auto viewMat = m_camera.ViewMatrix();
		UpdateGlobals(projectionMat, viewMat);

		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr && m_lights[l].m_updateShadowmap)
			{
				RenderShadowmap(d, m_lights[l]);
			}
		}

		// lighting to main frame buffer
		{
			SDE_PROF_EVENT("RenderMainBuffer");
			d.DrawToFramebuffer(m_mainFramebuffer);
			d.SetViewport(glm::ivec2(0, 0), m_mainFramebuffer.Dimensions());

			// render opaques
			m_frameStats.m_totalOpaqueInstances = m_opaqueInstances.m_instances.size();
			int baseIndex = PrepareOpaqueInstances(m_opaqueInstances);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);				// no blending for opaques
			d.SetScissorEnabled(false);			// (don't) scissor me timbers
			DrawInstances(d, m_opaqueInstances, baseIndex, true);
			m_frameStats.m_renderedOpaqueInstances = m_opaqueInstances.m_instances.size();

			// render transparents
			m_frameStats.m_totalTransparentInstances = m_transparentInstances.m_instances.size();
			baseIndex = PrepareTransparentInstances(m_transparentInstances);
			d.SetDepthState(true, false);		// enable z-test, disable write
			d.SetBlending(true);
			DrawInstances(d, m_transparentInstances, baseIndex, true);
			m_frameStats.m_renderedTransparentInstances = m_transparentInstances.m_instances.size();
		}

		// blit main buffer to backbuffer
		d.SetDepthState(false, false);
		d.SetBlending(true);
		auto blitShader = m_shaders->GetShader(m_blitShader);
		if (blitShader)
		{
			m_targetBlitter.TargetToBackbuffer(d, m_mainFramebuffer, *blitShader, m_windowSize);
		}
	}

	int Renderer::PrepareCulledShadowInstances(InstanceList& visibleInstances)
	{
		SDE_PROF_EVENT();
		std::sort(visibleInstances.m_instances.begin(), visibleInstances.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2) -> bool {
			if ((uintptr_t)q1.m_shader < (uintptr_t)q2.m_shader)	// shader
			{
				return true;
			}
			else if ((uintptr_t)q1.m_shader > (uintptr_t)q2.m_shader)
			{
				return false;
			}
			auto q1Mesh = reinterpret_cast<uintptr_t>(q1.m_mesh);	// mesh
			auto q2Mesh = reinterpret_cast<uintptr_t>(q2.m_mesh);
			if (q1Mesh < q2Mesh)
			{
				return true;
			}
			else if (q1Mesh > q2Mesh)
			{
				return false;
			}
			return false;
		});
		return PopulateInstanceBuffers(visibleInstances);
	}

	int Renderer::PrepareShadowInstances(glm::mat4 lightViewProj, InstanceList& visibleInstances)
	{
		SDE_PROF_EVENT();
		Frustum viewFrustum(lightViewProj);
		visibleInstances.m_instances.clear();
		if (m_cullingEnabled)
		{
			CullInstances(m_allShadowCasterInstances, &visibleInstances, &viewFrustum);
		}
		else
		{
			visibleInstances = m_allShadowCasterInstances;
		}
		return PrepareCulledShadowInstances(visibleInstances);
	}

	int Renderer::PrepareTransparentInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		InstanceList visibleInstances;
		const auto projectionMat = m_camera.ProjectionMatrix();
		const auto viewMat = m_camera.ViewMatrix();
		Frustum viewFrustum(projectionMat * viewMat);
		if (m_cullingEnabled)
		{
			CullInstances(list, &visibleInstances, &viewFrustum);
		}
		else
		{
			visibleInstances = list;
		}
		{
			SDE_PROF_EVENT("Sort");
			std::sort(visibleInstances.m_instances.begin(), visibleInstances.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2) -> bool {
				if (q1.m_distanceToCamera < q2.m_distanceToCamera)	// back to front
				{
					return false;
				}
				else if (q1.m_distanceToCamera > q2.m_distanceToCamera)
				{
					return true;
				}
				if ((uintptr_t)q1.m_shader < (uintptr_t)q2.m_shader)	// shader
				{
					return true;
				}
				else if ((uintptr_t)q1.m_shader > (uintptr_t)q2.m_shader)
				{
					return false;
				}
				auto q1Mesh = reinterpret_cast<uintptr_t>(q1.m_mesh);	// mesh
				auto q2Mesh = reinterpret_cast<uintptr_t>(q2.m_mesh);
				if (q1Mesh < q2Mesh)
				{
					return true;
				}
				else if (q1Mesh > q2Mesh)
				{
					return false;
				}
				return false;
			});
		}
		list.m_instances = std::move(visibleInstances.m_instances);
		return PopulateInstanceBuffers(list);
	}

	int Renderer::PrepareOpaqueInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		InstanceList allVisibleInstances;
		Frustum viewFrustum(m_camera.ProjectionMatrix() * m_camera.ViewMatrix());
		if (m_cullingEnabled)
		{
			CullInstances(list, &allVisibleInstances, &viewFrustum);
		}
		else
		{
			allVisibleInstances = list;
		}
		{
			SDE_PROF_EVENT("Sort");
			std::sort(allVisibleInstances.m_instances.begin(), allVisibleInstances.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2) -> bool {
				if ((uintptr_t)q1.m_shader < (uintptr_t)q2.m_shader)	// shader
				{
					return true;
				}
				else if ((uintptr_t)q1.m_shader > (uintptr_t)q2.m_shader)
				{
					return false;
				}
				auto q1Mesh = reinterpret_cast<uintptr_t>(q1.m_mesh);	// mesh
				auto q2Mesh = reinterpret_cast<uintptr_t>(q2.m_mesh);
				if (q1Mesh < q2Mesh)
				{
					return true;
				}
				else if (q1Mesh > q2Mesh)
				{
					return false;
				}
				if (q1.m_distanceToCamera < q2.m_distanceToCamera)	// front to back
				{
					return true;
				}
				else if (q1.m_distanceToCamera > q2.m_distanceToCamera)
				{
					return false;
				}
				return false;
			});
		}
		list.m_instances = std::move(allVisibleInstances.m_instances);
		return PopulateInstanceBuffers(list);
	}

	void Renderer::CullInstances(const InstanceList& srcInstances, InstanceList* results, const Frustum* frustums, int listCount)
	{
		SDE_PROF_EVENT();
		const int instancesPerThread = 1024;
		std::atomic<int> jobsRemaining = 0;
		std::vector<std::atomic<int>> totalVisibleInstances(listCount);
		std::vector<std::vector<std::unique_ptr<InstanceList>>> allResults;			// ew
		{
			SDE_PROF_EVENT("SubmitJobs");
			for (int listIndex = 0; listIndex < listCount; ++listIndex)
			{
				std::vector<std::unique_ptr<InstanceList>> jobResults;
				const int instanceCount = srcInstances.m_instances.size();
				for (int i = 0; i < instanceCount; i += instancesPerThread)
				{
					const int startIndex = i;
					const int endIndex = std::min(i + instancesPerThread, instanceCount);
					auto visibleInstances = std::make_unique<InstanceList>();		// instance list per job
					visibleInstances->m_instances.reserve(instancesPerThread);
					auto instanceListPtr = visibleInstances.get();
					jobResults.push_back(std::move(visibleInstances));
					auto cullJob = [&jobsRemaining, &totalVisibleInstances, instanceListPtr, &srcInstances, frustums, startIndex, endIndex, listIndex](void*)
					{
						SDE_PROF_EVENT("Culling");
						for (int i = startIndex; i < endIndex; ++i)
						{
							if (frustums[listIndex].IsBoxVisible(srcInstances.m_instances[i].m_aabbMin, srcInstances.m_instances[i].m_aabbMax, srcInstances.m_instances[i].m_transform))
							{
								instanceListPtr->m_instances.emplace_back(srcInstances.m_instances[i]);
							}
						}
						totalVisibleInstances[listIndex] += instanceListPtr->m_instances.size();
						jobsRemaining--;
					};
					jobsRemaining++;
					m_jobSystem->PushJob(cullJob);
				}
				allResults.emplace_back(std::move(jobResults));
			}
		}
		{
			SDE_PROF_STALL("WaitForCulling");
			while (jobsRemaining > 0)
			{
				Core::Thread::Sleep(0);
			}
		}
		{
			SDE_PROF_EVENT("PushResults");
			for (int listIndex = 0; listIndex < listCount; ++listIndex)
			{
				results[listIndex].m_instances.reserve(totalVisibleInstances[listIndex]);
				for (const auto& result : allResults[listIndex])
				{
					SDE_PROF_EVENT("Insert");
					results[listIndex].m_instances.insert(results[listIndex].m_instances.end(), result->m_instances.begin(), result->m_instances.end());
				}
			}
		}
	}
}