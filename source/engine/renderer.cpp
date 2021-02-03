#include "renderer.h"
#include "core/log.h"
#include "core/profiler.h"
#include "core/string_hashing.h"
#include "render/shader_program.h"
#include "render/shader_binary.h"
#include "render/device.h"
#include "render/mesh.h"
#include "render/material.h"
#include "mesh_instance.h"
#include "model_manager.h"
#include "shader_manager.h"
#include "model.h"
#include "material_helpers.h"
#include <algorithm>
#include <map>

namespace Engine
{
	const uint64_t c_maxInstances = 1024 * 128;
	const uint64_t c_maxLights = 64;
	const uint32_t c_maxShadowMaps = 8;

	struct LightInfo
	{
		glm::vec4 m_colourAndAmbient;	// rgb=colour, a=ambient multiplier
		glm::vec4 m_position;
		glm::vec3 m_attenuation;		// const, linear, quad
		glm::vec3 m_shadowParams;		// enabled, far plane, index?
		glm::mat4 m_lightSpaceMatrix;	// for shadow calculations
	};

	struct GlobalUniforms
	{
		glm::mat4 m_viewProjMat;
		glm::vec4 m_cameraPosition;		// world-space
		LightInfo m_lights[c_maxLights];
		int m_lightCount;
		float m_hdrExposure;
		float m_shadowBias;
		float m_cubeShadowBias;
	};

	std::map<std::string, TextureHandle> g_defaultTextures;

	Renderer::Renderer(TextureManager* ta, ModelManager* mm, ShaderManager* sm, glm::ivec2 windowSize)
		: m_textures(ta)
		, m_models(mm)
		, m_shaders(sm)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
	{
		g_defaultTextures["DiffuseTexture"] = m_textures->LoadTexture("white.bmp");
		g_defaultTextures["NormalsTexture"] = m_textures->LoadTexture("default_normalmap.png");
		g_defaultTextures["SpecularTexture="] = m_textures->LoadTexture("white.bmp");
		m_blitShader = m_shaders->LoadShader("Basic Blit", "basic_blit.vs", "basic_blit.fs");
		{
			SDE_PROF_EVENT("Create Buffers");
			CreateInstanceList(m_opaqueInstances, c_maxInstances);
			CreateInstanceList(m_transparentInstances, c_maxInstances);
			CreateInstanceList(m_allShadowCasterInstances, c_maxInstances);
			CreateInstanceList(m_visibleShadowInstances, c_maxInstances);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferType::UniformData, Render::RenderBufferModification::Dynamic, true);
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

	void Renderer::CreateInstanceList(InstanceList& newlist, uint32_t maxInstances)
	{
		SDE_PROF_EVENT();
		newlist.m_instances.reserve(maxInstances);
		newlist.m_transforms.Create(maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
		newlist.m_colours.Create(maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
	}

	void Renderer::Reset() 
	{ 
		m_opaqueInstances.m_instances.clear();
		m_transparentInstances.m_instances.clear();
		m_allShadowCasterInstances.m_instances.clear();
		m_lights.clear();
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

	void Renderer::SubmitInstance(InstanceList& list, glm::vec3 cameraPos, glm::mat4 transform, glm::vec4 colour, const Render::Mesh& mesh, const struct ShaderHandle& shader)
	{
		float distanceToCamera = glm::length(glm::vec3(transform[3]) - cameraPos);
		const auto foundShader = m_shaders->GetShader(shader);
		list.m_instances.push_back({ transform, colour, foundShader, &mesh, distanceToCamera });
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
				if (shadowShader.m_index != -1)
				{
					SubmitInstance(m_allShadowCasterInstances, m_camera.Position(), transform, colour, *part.m_mesh, shadowShader);
				}

				bool isTransparent = colour.a != 1.0f;
				if (!isTransparent)
				{
					isTransparent = IsMeshTransparent(*part.m_mesh, *m_textures);
				}
				InstanceList& instances = isTransparent ? m_transparentInstances : m_opaqueInstances;
				const glm::mat4 instanceTransform = transform * part.m_transform;
				SubmitInstance(instances, m_camera.Position(), instanceTransform, colour, *part.m_mesh, shader);
			}
		}
	}

	glm::mat4 GetLightspaceTransform(const Light& l)
	{
		if (l.m_position.w == 0.0f)
		{
			// todo - parameterise
			static float c_nearPlane = 1.0f;
			static float c_farPlane = 1000.0f;
			static float c_orthoDims = 400.0f;
			glm::mat4 lightProjection = glm::ortho(-c_orthoDims, c_orthoDims, -c_orthoDims, c_orthoDims, c_nearPlane, c_farPlane);
			glm::mat4 lightView = glm::lookAt(glm::vec3(l.m_position), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			return lightProjection * lightView;
		}
		else
		{
			// todo: parameterise
			auto lightPos = glm::vec3(l.m_position);
			float aspect = l.m_shadowMap->Dimensions().x / (float)l.m_shadowMap->Dimensions().y;
			float near = 0.1f;
			float far = 500.0f;
			return glm::perspective(glm::radians(90.0f), aspect, near, far);
		}
	}

	void Renderer::SetLight(glm::vec4 positionOrDir, glm::vec3 colour, float ambientStr, glm::vec3 attenuation,	Render::FrameBuffer& sm)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = positionOrDir;
		newLight.m_attenuation = attenuation;
		newLight.m_shadowMap = &sm;
		newLight.m_lightspaceMatrix = GetLightspaceTransform(newLight);
		m_lights.push_back(newLight);
	}

	void Renderer::SetLight(glm::vec4 positionOrDir, glm::vec3 colour, float ambientStr, glm::vec3 attenuation)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = positionOrDir;
		newLight.m_attenuation = attenuation;
		m_lights.push_back(newLight);
	}

	void Renderer::PopulateInstanceBuffers(InstanceList& list)
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
		if (list.m_instances.size() > 0)
		{
			list.m_transforms.SetData(0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
			list.m_colours.SetData(0, instanceColours.size() * sizeof(glm::vec4), instanceColours.data());
		}
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
			globals.m_lights[l].m_attenuation = m_lights[l].m_attenuation;
			if (m_lights[l].m_shadowMap)
			{
				globals.m_lights[l].m_shadowParams.x = 1.0f;
				globals.m_lights[l].m_shadowParams.y = m_lights[l].m_position.w == 0.0f ? 1000.0f : 500.0f;		// far plane hacks todo
				globals.m_lights[l].m_shadowParams.z = m_lights[l].m_position.w == 0.0f ? shadowMapIndex++ : cubeShadowMapIndex++;
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
		globals.m_shadowBias = m_shadowBias;
		globals.m_cubeShadowBias = m_cubeShadowBias;
		m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);
	}

	uint32_t Renderer::BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit)
	{
		uint32_t shadowMapIndex = 0;
		uint32_t cubeShadowMapIndex = 0;
		char samplerNameBuffer[256] = { '\0' };
		for (int l = 0; l < m_lights.size() && l < c_maxLights && shadowMapIndex < c_maxShadowMaps && cubeShadowMapIndex < c_maxShadowMaps; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr)
			{
				if (m_lights[l].m_position.w == 0.0f)
				{
					sprintf_s(samplerNameBuffer, "ShadowMaps[%d]", shadowMapIndex++);
					uint32_t uniformHandle = shader.GetUniformHandle(samplerNameBuffer);
					if (uniformHandle != -1)
					{
						d.SetSampler(uniformHandle, m_lights[l].m_shadowMap->GetDepthStencil()->GetHandle(), textureUnit++);
					}
				}
				else
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
		return textureUnit;
	}

	void Renderer::DrawInstances(Render::Device& d, const InstanceList& list, bool bindShadowmaps, Render::UniformBuffer* uniforms)
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

				// bind vertex array + instancing streams immediately after mesh vertex streams
				m_frameStats.m_vertexArrayBinds++;
				int instancingSlotIndex = theMesh->GetVertexArray().GetStreamCount();
				d.BindVertexArray(theMesh->GetVertexArray());
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, 0, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, sizeof(float) * 4, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, sizeof(float) * 8, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_transforms, instancingSlotIndex++, 4, sizeof(float) * 12, 4);
				d.BindInstanceBuffer(theMesh->GetVertexArray(), list.m_colours, instancingSlotIndex++, 4, 0);

				// apply mesh material uniforms and samplers
				uint32_t textureUnit = 0;
				textureUnit = ApplyMaterial(d, *theShader, theMesh->GetMaterial(), *m_textures, g_defaultTextures, textureUnit++);
				if (bindShadowmaps)
				{
					textureUnit = BindShadowmaps(d, *theShader, textureUnit);
				}

				// draw the chunks
				for (const auto& chunk : theMesh->GetChunks())
				{
					uint32_t firstIndex = (uint32_t)(firstInstance - list.m_instances.begin());
					d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex);
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

		if (l.m_position.w == 0.0f)
		{
			PrepareShadowInstances(l.m_lightspaceMatrix, m_visibleShadowInstances);

			Render::UniformBuffer lightMatUniforms;
			lightMatUniforms.SetValue("ShadowLightSpaceMatrix", l.m_lightspaceMatrix);
			lightMatUniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
			d.DrawToFramebuffer(*l.m_shadowMap);
			d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
			d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);
			d.SetScissorEnabled(false);
			DrawInstances(d, m_visibleShadowInstances, false, &lightMatUniforms);
		}
		else
		{
			Render::UniformBuffer uniforms;
			SDE_PROF_EVENT("RenderShadowCubemap");
			
			const auto pos3 = glm::vec3(l.m_position);
			const glm::mat4 shadowTransforms[] = {
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
				l.m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0,0.0f))
			};
			for (uint32_t cubeFace = 0; cubeFace < 6; ++cubeFace)
			{
				PrepareShadowInstances(shadowTransforms[cubeFace], m_visibleShadowInstances);
				d.DrawToFramebuffer(*l.m_shadowMap, cubeFace);
				d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
				d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
				d.SetBackfaceCulling(true, true);	// backface culling, ccw order
				d.SetBlending(false);	
				d.SetScissorEnabled(false);	
				uniforms.SetValue("ShadowLightSpaceMatrix", shadowTransforms[cubeFace]);
				uniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
				DrawInstances(d, m_visibleShadowInstances, false, &uniforms);
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

		// setup global constants
		auto projectionMat = glm::perspectiveFov(glm::radians(70.0f), (float)m_windowSize.x, (float)m_windowSize.y, 0.1f, 1000.0f);
		auto viewMat = glm::lookAt(m_camera.Position(), m_camera.Target(), m_camera.Up());
		UpdateGlobals(projectionMat, viewMat);

		// render shadow maps
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr)
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
			PrepareOpaqueInstances(m_opaqueInstances);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);				// no blending for opaques
			d.SetScissorEnabled(false);			// (don't) scissor me timbers
			DrawInstances(d, m_opaqueInstances, true);

			// render transparents
			PrepareTransparentInstances(m_transparentInstances);
			d.SetDepthState(true, false);		// enable z-test, disable write
			d.SetBlending(true);
			DrawInstances(d, m_transparentInstances, true);
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

	void Renderer::PrepareShadowInstances(glm::mat4 lightViewProj, InstanceList& visibleInstances)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("FrustumCull");
			visibleInstances.m_instances.clear();
			visibleInstances.m_instances.reserve(m_allShadowCasterInstances.m_instances.size() >> 1);
			for (const auto& it : m_allShadowCasterInstances.m_instances)
			{
				visibleInstances.m_instances.push_back(it);
			}
		}
		{
			SDE_PROF_EVENT("Sort");
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
		}
		PopulateInstanceBuffers(visibleInstances);
	}

	void Renderer::PrepareShadowInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("Sort");
			std::sort(list.m_instances.begin(), list.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2) -> bool {
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
		PopulateInstanceBuffers(list);
	}

	void Renderer::PrepareTransparentInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		InstanceList visibleInstances;
		{
			SDE_PROF_EVENT("FrustumCull");
			visibleInstances.m_instances.reserve(list.m_instances.size() >> 1);
			for (const auto& it : list.m_instances)
			{
				visibleInstances.m_instances.push_back(it);
			}
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
		PopulateInstanceBuffers(list);
	}

	void Renderer::PrepareOpaqueInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		InstanceList visibleInstances;

		{
			SDE_PROF_EVENT("FrustumCull");
			visibleInstances.m_instances.reserve(list.m_instances.size() >> 1);
			for (const auto& it : list.m_instances)
			{
				visibleInstances.m_instances.push_back(it);
			}
		}

		{
			SDE_PROF_EVENT("Sort");
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
		list.m_instances = std::move(visibleInstances.m_instances);
		PopulateInstanceBuffers(list);
	}
}