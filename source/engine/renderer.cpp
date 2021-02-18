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
		glm::vec4 m_position;
		glm::vec3 m_direction;			// w = spotlight angle or pointlight radius used for attenuation	
		glm::vec3 m_attenuation;		// todo - delete
		glm::vec4 m_shadowParams;		// enabled, far plane, index, bias
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
		g_defaultTextures["SpecularTexture="] = m_textures->LoadTexture("white.bmp");
		m_blitShader = m_shaders->LoadShader("Basic Blit", "basic_blit.vs", "basic_blit.fs");
		{
			SDE_PROF_EVENT("Create Buffers");

			m_transforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);
			m_colours.Create(c_maxInstances * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic, true);

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

	void MakeAABBFromBounds(glm::vec3 oobbMin, glm::vec3 oobbMax, glm::mat4 transform, glm::vec3& aabbMin, glm::vec3& aabbMax)
	{
		glm::vec4 v[] = {
			{oobbMin.x,oobbMin.y,oobbMin.z,1.0f},	
			{oobbMax.x,oobbMin.y,oobbMin.z,1.0f},
			{oobbMax.x,oobbMin.y,oobbMax.z,1.0f},
			{oobbMin.x,oobbMin.y,oobbMax.z,1.0f},
			{oobbMin.x,oobbMax.y,oobbMin.z,1.0f},	
			{oobbMax.x,oobbMax.y,oobbMin.z,1.0f},
			{oobbMax.x,oobbMax.y,oobbMax.z,1.0f},
			{oobbMin.x,oobbMax.y,oobbMax.z,1.0f},
		};
		glm::vec4 bMin = { FLT_MAX,FLT_MAX ,FLT_MAX, 1.0f };
		glm::vec4 bMax = { -FLT_MAX,-FLT_MAX ,-FLT_MAX, 1.0f };
		for (auto& vert : v)
		{
			vert = transform * vert;
			bMin = glm::min(vert, bMin);
			bMax = glm::max(vert, bMax);
		}
		aabbMin = glm::vec3(bMin);
		aabbMax = glm::vec3(bMax);
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
				glm::vec3 boundsMin, boundsMax;
				MakeAABBFromBounds(part.m_boundsMin, part.m_boundsMax, instanceTransform, boundsMin, boundsMax);

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

	void Renderer::SetLight(glm::vec4 posAndType, glm::vec3 direction, glm::vec3 colour, float ambientStr, glm::vec3 attenuation, Render::FrameBuffer& sm, float bias, glm::mat4 shadowMatrix, bool updateShadowmap)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = posAndType;
		newLight.m_direction = direction;
		newLight.m_attenuation = attenuation;
		newLight.m_shadowMap = &sm;
		newLight.m_lightspaceMatrix = shadowMatrix;
		newLight.m_shadowBias = bias;
		newLight.m_updateShadowmap = updateShadowmap;
		m_lights.push_back(newLight);
	}

	void Renderer::SetLight(glm::vec4 posAndType, glm::vec3 direction, glm::vec3 colour, float ambientStr, glm::vec3 attenuation)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = posAndType;
		newLight.m_direction = direction;
		newLight.m_attenuation = attenuation;
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
			globals.m_lights[l].m_attenuation = m_lights[l].m_attenuation;
			bool hasShadowMap = m_lights[l].m_position.w == 0.0f ? shadowMapIndex < c_maxShadowMaps : cubeShadowMapIndex < c_maxShadowMaps;
			if (m_lights[l].m_shadowMap)
			{
				globals.m_lights[l].m_shadowParams.x = 1.0f;
				globals.m_lights[l].m_shadowParams.y = m_lights[l].m_position.w == 0.0f ? 1000.0f : 500.0f;		// far plane hacks todo
				globals.m_lights[l].m_shadowParams.z = m_lights[l].m_position.w == 0.0f ? shadowMapIndex++ : cubeShadowMapIndex++;
				globals.m_lights[l].m_shadowParams.w = m_lights[l].m_shadowBias;
				globals.m_lights[l].m_lightSpaceMatrix = m_lights[l].m_lightspaceMatrix;
			}
			else
			{
				globals.m_lights[l].m_shadowParams = { 0.0f,0.0f,0.0f,0.0f };
			}
		}
		globals.m_lightCount = static_cast<int>(std::min(m_lights.size(), c_maxLights));
		globals.m_cameraPosition = glm::vec4(m_camera.Position(), 0.0);
		globals.m_hdrExposure = m_hdrExposure;
		m_globalsUniformBuffer.SetData(0, sizeof(globals), &globals);
	}

	uint32_t Renderer::BindShadowmaps(Render::Device& d, Render::ShaderProgram& shader, uint32_t textureUnit)
	{
		uint32_t shadowMapIndex = 0;
		uint32_t cubeShadowMapIndex = 0;
		char samplerNameBuffer[256] = { '\0' };
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr)
			{
				if (m_lights[l].m_position.w == 0.0f && shadowMapIndex < c_maxShadowMaps)
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

		return textureUnit;
	}

	void Renderer::DrawInstances(Render::Device& d, const InstanceList& list, int baseIndex, bool bindShadowmaps, Render::UniformBuffer* uniforms)
	{
		// populate the global instance buffers

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

		if (l.m_position.w == 0.0f)		// directional
		{
			int baseIndex = PrepareShadowInstances(l.m_lightspaceMatrix, m_visibleShadowInstances);
			Render::UniformBuffer lightMatUniforms;
			lightMatUniforms.SetValue("ShadowLightSpaceMatrix", l.m_lightspaceMatrix);
			lightMatUniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
			d.DrawToFramebuffer(*l.m_shadowMap);
			d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
			d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);
			d.SetScissorEnabled(false);
			DrawInstances(d, m_visibleShadowInstances, baseIndex, false, &lightMatUniforms);
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
				int baseIndex = PrepareShadowInstances(shadowTransforms[cubeFace], m_visibleShadowInstances);
				d.DrawToFramebuffer(*l.m_shadowMap, cubeFace);
				d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
				d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
				d.SetBackfaceCulling(true, true);	// backface culling, ccw order
				d.SetBlending(false);	
				d.SetScissorEnabled(false);	
				uniforms.SetValue("ShadowLightSpaceMatrix", shadowTransforms[cubeFace]);
				uniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
				DrawInstances(d, m_visibleShadowInstances, baseIndex, false, &uniforms);
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
		auto projectionMat = m_camera.ProjectionMatrix();
		auto viewMat = m_camera.ViewMatrix();
		UpdateGlobals(projectionMat, viewMat);

		static int s_maxPerFrame = 16;
		static int s_nextUpdate = 0;
		int updatesRemaining = s_maxPerFrame;
		if (s_nextUpdate >= m_lights.size() || s_nextUpdate >= c_maxLights)
			s_nextUpdate = 0;
		for (int l = s_nextUpdate; l < m_lights.size() && l < c_maxLights && updatesRemaining > 0; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr)
			{
				RenderShadowmap(d, m_lights[l]);
				--updatesRemaining;
				s_nextUpdate = l + 1;
			}
		}

		// lighting to main frame buffer
		{
			SDE_PROF_EVENT("RenderMainBuffer");
			d.DrawToFramebuffer(m_mainFramebuffer);
			d.SetViewport(glm::ivec2(0, 0), m_mainFramebuffer.Dimensions());

			// render opaques
			int baseIndex = PrepareOpaqueInstances(m_opaqueInstances);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);				// no blending for opaques
			d.SetScissorEnabled(false);			// (don't) scissor me timbers
			DrawInstances(d, m_opaqueInstances, baseIndex, true);

			// render transparents
			baseIndex = PrepareTransparentInstances(m_transparentInstances);
			d.SetDepthState(true, false);		// enable z-test, disable write
			d.SetBlending(true);
			DrawInstances(d, m_transparentInstances, baseIndex, true);
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

	int Renderer::PrepareShadowInstances(glm::mat4 lightViewProj, InstanceList& visibleInstances)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("FrustumCull");
			Frustum viewFrustum(lightViewProj);
			visibleInstances.m_instances.clear();
			if (m_cullingEnabled)
			{
				CullInstances(viewFrustum, m_allShadowCasterInstances, visibleInstances);
			}
			else
			{
				visibleInstances = m_allShadowCasterInstances;
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
		return PopulateInstanceBuffers(visibleInstances);
	}

	int Renderer::PrepareTransparentInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		InstanceList visibleInstances;
		{
			SDE_PROF_EVENT("FrustumCull");
			const auto projectionMat = m_camera.ProjectionMatrix();
			const auto viewMat = m_camera.ViewMatrix();
			Frustum viewFrustum(projectionMat * viewMat);
			if (m_cullingEnabled)
			{
				CullInstances(viewFrustum, list, visibleInstances);
			}
			else
			{
				visibleInstances = list;
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
		return PopulateInstanceBuffers(list);
	}

	int Renderer::PrepareOpaqueInstances(InstanceList& list)
	{
		SDE_PROF_EVENT();
		InstanceList allVisibleInstances;
		{
			SDE_PROF_EVENT("FrustumCull");
			Frustum viewFrustum(m_camera.ProjectionMatrix() * m_camera.ViewMatrix());
			if (m_cullingEnabled)
			{
				CullInstances(viewFrustum, list, allVisibleInstances);
			}
			else
			{
				allVisibleInstances = list;
			}
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

	void Renderer::CullInstances(const Frustum& f, const InstanceList& srcInstances, InstanceList& results)
	{
		SDE_PROF_EVENT();
		static bool s_singleThreadCull = false;
		if (!s_singleThreadCull)
		{
			const int instanceCount = srcInstances.m_instances.size();
			const int instancesPerThread = 1024;
			std::atomic<int> jobsRemaining = 0;
			std::atomic<int> totalVisibleInstances = 0;
			std::vector<std::unique_ptr<InstanceList>> jobResults;
			for (int i = 0; i < instanceCount; i += instancesPerThread)
			{
				const int startIndex = i;
				const int endIndex = std::min(i + instancesPerThread, instanceCount);
				auto visibleInstances = std::make_unique<InstanceList>();
				visibleInstances->m_instances.reserve(instancesPerThread);
				auto instanceListPtr = visibleInstances.get();
				jobResults.push_back(std::move(visibleInstances));
				auto cullJob = [&jobsRemaining, &totalVisibleInstances, instanceListPtr, &srcInstances, f, startIndex, endIndex](void*)
				{
					SDE_PROF_EVENT("Culling");
					for (int i = startIndex; i < endIndex; ++i)
					{
						if (f.IsBoxVisible(srcInstances.m_instances[i].m_aabbMin, srcInstances.m_instances[i].m_aabbMax))
						{
							instanceListPtr->m_instances.push_back(srcInstances.m_instances[i]);
						}
					}
					totalVisibleInstances += instanceListPtr->m_instances.size();
					jobsRemaining--;
				};
				jobsRemaining++;
				m_jobSystem->PushJob(cullJob);
			}
			{
				SDE_PROF_EVENT("WaitForCulling");
				while (jobsRemaining > 0)
				{
					Core::Thread::Sleep(0);
				}
			}
			{
				SDE_PROF_EVENT("PushResults");
				results.m_instances.resize(totalVisibleInstances);
				int endIndex = 0;
				for (const auto& result : jobResults)
				{
					memcpy(&results.m_instances[endIndex], result->m_instances.data(), result->m_instances.size() * sizeof(Engine::MeshInstance));
					endIndex += result->m_instances.size();
				}
			}
		}
		else
		{
			for (const auto& it : srcInstances.m_instances)
			{
				if (f.IsBoxVisible(it.m_aabbMin, it.m_aabbMax))
				{
					results.m_instances.push_back(it);
				}
			}
		}
	}
}