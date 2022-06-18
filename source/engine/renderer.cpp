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
#include "system_manager.h"
#include <algorithm>
#include <map>

namespace Engine
{
	const uint64_t c_maxInstances = 1024 * 256;		// this needs to include all passes/shadowmap updates
	const uint64_t c_maxLights = 256;
	const uint32_t c_maxShadowMaps = 16;
	const int32_t c_bloomBufferSizeDivider = 2;	// windowsize/divider
	const uint32_t c_bloomBlurIterations = 10;
	const int c_msaaSamples = 4;

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

	DefaultTextures g_defaultTextures;
	std::vector<std::string> g_shadowSamplerNames, g_shadowCubeSamplerNames;

	Renderer::Renderer(JobSystem* js, glm::ivec2 windowSize)
		: m_jobSystem(js)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
		, m_mainFramebufferResolved(windowSize)
		, m_bloomBrightnessBuffer(windowSize)
	{
		auto tm = Engine::GetSystem<Engine::TextureManager>("Textures");
		auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		g_defaultTextures["DiffuseTexture"] = tm->LoadTexture("white.bmp");
		g_defaultTextures["NormalsTexture"] = tm->LoadTexture("default_normalmap.png");
		g_defaultTextures["SpecularTexture"] = tm->LoadTexture("white.bmp");
		m_blitShader = sm->LoadShader("Basic Blit", "basic_blit.vs", "basic_blit.fs");
		m_bloomBrightnessShader = sm->LoadShader("BloomBrightness", "basic_blit.vs", "bloom_brightness.fs");
		m_bloomBlurShader = sm->LoadShader("BloomBlur", "basic_blit.vs", "bloom_blur.fs");
		m_bloomCombineShader = sm->LoadShader("BloomCombine", "basic_blit.vs", "bloom_combine.fs");
		m_tonemapShader = sm->LoadShader("Tonemap", "basic_blit.vs", "tonemap.fs");
		{
			SDE_PROF_EVENT("Create Buffers");
			m_transforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferModification::Dynamic, true);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferModification::Dynamic, true);
			m_opaqueInstances.m_instances.reserve(c_maxInstances);
			m_transparentInstances.m_instances.reserve(c_maxInstances);
			m_allShadowCasterInstances.m_instances.reserve(c_maxInstances);
		}
		{
			SDE_PROF_EVENT("Create render targets");
			m_mainFramebuffer.SetMSAASamples(c_msaaSamples);
			m_mainFramebuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F16);
			m_mainFramebuffer.AddDepthStencil();
			if (!m_mainFramebuffer.Create())
			{
				SDE_LOG("Failed to create framebuffer!");
			}
			m_mainFramebufferResolved.AddColourAttachment(Render::FrameBuffer::RGBA_F16);
			if (!m_mainFramebufferResolved.Create())
			{
				SDE_LOG("Failed to create framebuffer!");
			}
			m_bloomBrightnessBuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F16);
			m_bloomBrightnessBuffer.Create();
			for (int i = 0; i < 2; ++i)
			{
				m_bloomBlurBuffers[i] = std::make_unique<Render::FrameBuffer>(windowSize / c_bloomBufferSizeDivider);
				m_bloomBlurBuffers[i]->AddColourAttachment(Render::FrameBuffer::RGBA_F16);
				m_bloomBlurBuffers[i]->Create();
			}
		}

		char nameBuffer[256] = { '\0' };
		for (int i = 0; i < c_maxShadowMaps; ++i)
		{
			sprintf_s(nameBuffer, "ShadowMaps[%d]", i);
			g_shadowSamplerNames.push_back(nameBuffer);
			sprintf_s(nameBuffer, "ShadowCubeMaps[%d]", i);
			g_shadowCubeSamplerNames.push_back(nameBuffer);
		}
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

	__m128i ShadowCasterSortKey(const ShaderHandle& shader, const Render::VertexArray* va)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t ptrHigh = static_cast<uint32_t>((vaPtr & 0xffffffff00000000) >> 32);
		uint32_t ptrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));
		__m128i result = _mm_set_epi32(shader.m_index, 0, ptrHigh, ptrLow);
		return result;
	}

	__m128i OpaqueSortKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::Material* instanceMat)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));
		
		const void* matPtrVoid = static_cast<const void*>(instanceMat);
		uintptr_t matPtr = reinterpret_cast<uintptr_t>(matPtrVoid);
		uint32_t matPtrLow = static_cast<uint32_t>((matPtr & 0x00000000ffffffff));
		
		__m128i result = _mm_set_epi32(shader.m_index, vaPtrLow, matPtrLow, 0);

		return result;
	}

	__m128i TransparentSortKey(const ShaderHandle& shader, const Render::Mesh& mesh, const Render::Material* instanceMat, float distanceToCamera)
	{
		const Render::VertexArray* va = &mesh.GetVertexArray();
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));

		const void* matPtrVoid = static_cast<const void*>(instanceMat);
		uintptr_t matPtr = reinterpret_cast<uintptr_t>(matPtrVoid);
		uint32_t matPtrLow = static_cast<uint32_t>((matPtr & 0x00000000ffffffff));

		uint32_t distanceToCameraAsInt = static_cast<uint32_t>(distanceToCamera * 100.0f);
		__m128i result = _mm_set_epi32(distanceToCameraAsInt, shader.m_index, vaPtrLow, matPtrLow);

		return result;
	}

	// https://stackoverflow.com/questions/56341434/compare-two-m128i-values-for-total-order/56346628
	inline bool SortKeyLessThan(__m128i a, __m128i b)
	{
		/* Compare 8-bit lanes for ( a < b ), store the bits in the low 16 bits of the
		   scalar value: */
		const int less = _mm_movemask_epi8(_mm_cmplt_epi8(a, b));

		/* Compare 8-bit lanes for ( a > b ), store the bits in the low 16 bits of the
		   scalar value: */
		const int greater = _mm_movemask_epi8(_mm_cmpgt_epi8(a, b));

		/* It's counter-intuitive, but this scalar comparison does the right thing.
		   Essentially, integer comparison searches for the most significant bit that
		   differs... */
		return less > greater;
	}

	void Renderer::SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, 
		const Render::VertexArray* va, const Render::RenderBuffer* ib, const Render::MeshChunk* chunks, uint32_t chunkCount, const Render::Material* meshMaterial,
		const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat)
	{
		static auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		const auto foundShader = sm->GetShader(shader);
		list.m_instances.emplace_back(std::move(MeshInstance{ sortKey, trns, aabbMin, aabbMax, foundShader, va, ib, chunks, chunkCount, meshMaterial, instanceMat }));
	}

	void Renderer::SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const Render::Mesh& mesh, const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat)
	{
		SubmitInstance(list, sortKey, trns, 
			&mesh.GetVertexArray(), mesh.GetIndexBuffer().get(), &mesh.GetChunks()[0], mesh.GetChunks().size(), &mesh.GetMaterial(),
			shader, aabbMin, aabbMax, instanceMat);
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, glm::vec3 boundsMin, glm::vec3 boundsMax, const Render::Material* instanceMat)
	{
		static auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		bool castShadow = mesh.GetMaterial().GetCastsShadows();
		bool isTransparent = mesh.GetMaterial().GetIsTransparent();

		if (instanceMat != nullptr)
		{
			castShadow &= instanceMat->GetCastsShadows();
			isTransparent |= instanceMat->GetIsTransparent();
		}

		if (castShadow)
		{
			auto shadowShader = sm->GetShadowsShader(shader);
			if (shadowShader.m_index != (uint32_t)-1)
			{
				auto sortKey = ShadowCasterSortKey(shadowShader, &mesh.GetVertexArray());
				SubmitInstance(m_allShadowCasterInstances, sortKey, transform, mesh, shadowShader, boundsMin, boundsMax);
			}
		}

		if (isTransparent)
		{
			const float distanceToCamera = glm::length(glm::vec3(transform[3]) - m_camera.Position());
			__m128i sortKey = TransparentSortKey(shader, mesh, instanceMat, distanceToCamera);
			SubmitInstance(m_transparentInstances, sortKey, transform, mesh, shader, boundsMin, boundsMax, instanceMat);
		}
		else
		{
			__m128i sortKey = OpaqueSortKey(shader, &mesh.GetVertexArray(), instanceMat);
			SubmitInstance(m_opaqueInstances, sortKey, transform, mesh, shader, boundsMin, boundsMax, instanceMat);
		}
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, const Render::Material* instanceMat)
	{
		SubmitInstance(transform, mesh, shader, { -FLT_MAX, -FLT_MAX, -FLT_MAX }, { FLT_MAX, FLT_MAX, FLT_MAX }, instanceMat);
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader, const Render::Material* instanceMat)
	{
		static ModelManager* mm = Engine::GetSystem<ModelManager>("Models");
		static auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		const auto theModel = mm->GetModel(model);
		const auto theShader = sm->GetShader(shader);
		ShaderHandle shadowShader = ShaderHandle::Invalid();
		//const Render::VertexArray* va = mm->GetVertexArray();
		//const Render::RenderBuffer* ib = mm->GetIndexBuffer();

		bool castShadow = true;
		if (instanceMat != nullptr)
		{
			castShadow &= instanceMat->GetCastsShadows();
		}
		if (castShadow)
		{
			shadowShader = sm->GetShadowsShader(shader);
		}

		if (theModel != nullptr && theShader != nullptr)
		{
			//auto shadowSortKey = ShadowCasterSortKey(shadowShader, va);
			//auto opaqueSortKey = OpaqueSortKey(shader, va, instanceMat);
			//for (const auto& meshPart : theModel->MeshParts())
			//{
			//	const glm::mat4 instanceTransform = transform * meshPart.m_transform;
			//	glm::vec3 boundsMin = meshPart.m_boundsMin, boundsMax = meshPart.m_boundsMax;
			//	if (castShadow && shadowShader.m_index != (uint32_t)-1)
			//	{
			//		SubmitInstance(m_allShadowCasterInstances, shadowSortKey, instanceTransform, 
			//			va, ib, &meshPart.m_chunks[0], (uint32_t)meshPart.m_chunks.size(), 
			//			&meshPart.m_material, shadowShader, boundsMin, boundsMax, instanceMat);
			//	}
			//	SubmitInstance(m_opaqueInstances, opaqueSortKey, instanceTransform, va, ib, &meshPart.m_chunks[0], (uint32_t)meshPart.m_chunks.size(),
			//		&meshPart.m_material, shadowShader, boundsMin, boundsMax, instanceMat);
			//}

			for (const auto& part : theModel->Parts())
			{
				const glm::mat4 instanceTransform = transform * part.m_transform;
				glm::vec3 boundsMin = part.m_boundsMin, boundsMax = part.m_boundsMax;
				if (castShadow && shadowShader.m_index != (uint32_t)-1)
				{
					auto sortKey = ShadowCasterSortKey(shadowShader, &part.m_mesh->GetVertexArray());
					SubmitInstance(m_allShadowCasterInstances, sortKey, instanceTransform, *part.m_mesh, shadowShader, boundsMin, boundsMax);
				}

				bool isTransparent = part.m_mesh->GetMaterial().GetIsTransparent();
				if (instanceMat != nullptr)
				{
					isTransparent |= instanceMat->GetIsTransparent();
				}

				if (isTransparent)
				{
					const float distanceToCamera = glm::length(glm::vec3(instanceTransform[3]) - m_camera.Position());
					__m128i sortKey = TransparentSortKey(shader, *part.m_mesh, instanceMat, distanceToCamera);
					SubmitInstance(m_transparentInstances, sortKey, instanceTransform, *part.m_mesh, shader, boundsMin, boundsMax, instanceMat);
				}
				else
				{
					__m128i sortKey = OpaqueSortKey(shader, &part.m_mesh->GetVertexArray(), instanceMat);
					SubmitInstance(m_opaqueInstances, sortKey, instanceTransform, *part.m_mesh, shader, boundsMin, boundsMax, instanceMat);
				}
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
		for (const auto& c : list.m_instances)
		{
			instanceTransforms.push_back(c.m_transform);
		}

		// copy the instance buffers to gpu
		int instanceIndex = -1;
		if (list.m_instances.size() > 0 && m_nextInstance + list.m_instances.size() < c_maxInstances)
		{
			m_transforms.SetData(m_nextInstance * sizeof(glm::mat4), instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
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
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr)
			{
				if (m_lights[l].m_position.w == 0.0f || m_lights[l].m_position.w == 2.0f && shadowMapIndex < c_maxShadowMaps)
				{
					uint32_t uniformHandle = shader.GetUniformHandle(g_shadowSamplerNames[shadowMapIndex++].c_str());
					if (uniformHandle != -1)
					{
						d.SetSampler(uniformHandle, m_lights[l].m_shadowMap->GetDepthStencil()->GetResidentHandle());
					}
				}
				else if(cubeShadowMapIndex < c_maxShadowMaps)
				{
					uint32_t uniformHandle = shader.GetUniformHandle(g_shadowCubeSamplerNames[cubeShadowMapIndex++].c_str());
					if (uniformHandle != -1)
					{
						d.SetSampler(uniformHandle, m_lights[l].m_shadowMap->GetDepthStencil()->GetResidentHandle());
					}
				}
			}
		}
		// make sure to reset any we are not using or opengl will NOT be happy
		for (int l = shadowMapIndex; l < c_maxShadowMaps; ++l)
		{
			uint32_t uniformHandle = shader.GetUniformHandle(g_shadowSamplerNames[shadowMapIndex++].c_str());
			if (uniformHandle != -1)
			{
				d.SetSampler(uniformHandle, 0);
			}
		}
		for (int l = cubeShadowMapIndex; l < c_maxShadowMaps; ++l)
		{
			uint32_t uniformHandle = shader.GetUniformHandle(g_shadowCubeSamplerNames[cubeShadowMapIndex++].c_str());
			if (uniformHandle != -1)
			{
				d.SetSampler(uniformHandle, 0);
			}
		}

		return textureUnit;
	}

	void Renderer::DrawInstances(Render::Device& d, const InstanceList& list, int baseIndex, bool bindShadowmaps, Render::UniformBuffer* uniforms)
	{
		SDE_PROF_EVENT();
		auto firstInstance = list.m_instances.begin();
		const Render::ShaderProgram* lastShaderUsed = nullptr;	// avoid setting the same shader
		const Render::Mesh* lastMeshUsed = nullptr;				// avoid binding the same vertex arrays
		const Render::Material* lastInstanceMaterial = nullptr;	// avoid setting instance materials for the same mesh/shaders
		const Render::VertexArray* lastVAUsed = nullptr;
		const Render::RenderBuffer* lastIBUsed = nullptr;
		while (firstInstance != list.m_instances.end())
		{
			// Batch by shader, va/ib, mesh and instance material
			auto lastMeshInstance = std::find_if(firstInstance, list.m_instances.end(), [firstInstance](const MeshInstance& m) -> bool {
				return  m.m_va != firstInstance->m_va || 
					m.m_ib != firstInstance->m_ib || 
					m.m_shader != firstInstance->m_shader || 
					m.m_meshMaterial != firstInstance->m_meshMaterial ||
					m.m_instanceMaterial != firstInstance->m_instanceMaterial;
			});
			auto instanceCount = (uint32_t)(lastMeshInstance - firstInstance);
			Render::ShaderProgram* theShader = firstInstance->m_shader;
			if (theShader != nullptr)
			{
				m_frameStats.m_batchesDrawn++;

				// bind shader + globals UBO
				if (theShader != lastShaderUsed)
				{
					m_frameStats.m_shaderBinds++;
					d.BindShaderProgram(*theShader);
					d.BindUniformBufferIndex(*theShader, "Globals", 0);
					d.SetUniforms(*theShader, m_globalsUniformBuffer, 0);
					d.BindStorageBuffer(0, m_transforms);		// bind instancing data once per shader
					if (uniforms != nullptr)
					{
						uniforms->Apply(d, *theShader);
					}
					if (bindShadowmaps)
					{
						BindShadowmaps(d, *theShader, 0);
					}

					// shader change = reset everything
					lastShaderUsed = theShader;
					lastMeshUsed = nullptr;
					lastInstanceMaterial = nullptr;
					lastVAUsed = nullptr;
					lastIBUsed = nullptr;
				}

				// apply mesh material uniforms and samplers
				if (firstInstance->m_meshMaterial != nullptr)
				{
					ApplyMaterial(d, *theShader, *firstInstance->m_meshMaterial, &g_defaultTextures);
				}

				// apply instance material uniforms and samplers (materials can be shared across instances!)
				auto instanceMaterial = firstInstance->m_instanceMaterial;
				if (instanceMaterial != nullptr && instanceMaterial != lastInstanceMaterial)
				{
					ApplyMaterial(d, *theShader, *instanceMaterial, &g_defaultTextures);
					lastInstanceMaterial = instanceMaterial;
				}

				if (firstInstance->m_va != lastVAUsed)
				{
					m_frameStats.m_vertexArrayBinds++;
					d.BindVertexArray(*firstInstance->m_va);
					lastVAUsed = firstInstance->m_va;
				}

				// draw the chunks
				if (firstInstance->m_ib != nullptr)
				{
					if (firstInstance->m_ib != lastIBUsed)
					{
						d.BindIndexBuffer(*firstInstance->m_ib);
						lastIBUsed = firstInstance->m_ib;
					}
					
					for(uint32_t c=0; c<firstInstance->m_chunkCount;++c)
					{
						const auto& chunk = firstInstance->m_chunks[c];
						uint32_t firstIndex = (uint32_t)(firstInstance - list.m_instances.begin());
						d.DrawPrimitivesInstancedIndexed(chunk.m_primitiveType,	chunk.m_firstVertex, chunk.m_vertexCount,
							instanceCount, firstIndex + baseIndex);
						m_frameStats.m_drawCalls++;
						m_frameStats.m_totalVertices += (uint64_t)chunk.m_vertexCount * instanceCount;
					}
				}
				else
				{
					for (uint32_t c = 0; c < firstInstance->m_chunkCount; ++c)
					{
						const auto& chunk = firstInstance->m_chunks[c];
						uint32_t firstIndex = (uint32_t)(firstInstance - list.m_instances.begin());
						d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex + baseIndex);
						m_frameStats.m_drawCalls++;
						m_frameStats.m_totalVertices += (uint64_t)chunk.m_vertexCount * instanceCount;
					}
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
			d.SetWireframeDrawing(m_showWireframe);
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
			if (m_showWireframe)
			{
				d.SetWireframeDrawing(false);
			}
		}

		Render::FrameBuffer* mainFb = &m_mainFramebuffer;
		if(m_mainFramebuffer.GetMSAASamples() > 1)
		{
			SDE_PROF_EVENT("ResolveMSAA");
			m_mainFramebuffer.Resolve(m_mainFramebufferResolved);
			mainFb = &m_mainFramebufferResolved;
		}

		// blit to bloom brightness buffer
		d.SetDepthState(false, false);
		d.SetBlending(false);
		static auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		auto brightnessShader = sm->GetShader(m_bloomBrightnessShader);
		if (brightnessShader)
		{
			SDE_PROF_EVENT("BloomBrightness");
			d.BindShaderProgram(*brightnessShader);		// must happen before setting uniforms
			auto threshold = brightnessShader->GetUniformHandle("BrightnessThreshold");
			if (threshold != -1)
				d.SetUniformValue(threshold, m_bloomThreshold);
			auto multi = brightnessShader->GetUniformHandle("BrightnessMulti");
			if (multi != -1)
				d.SetUniformValue(multi, m_bloomMultiplier);
			m_targetBlitter.TargetToTarget(d, *mainFb, m_bloomBrightnessBuffer, *brightnessShader);
		}

		// gaussian blur 
		auto blurShader = sm->GetShader(m_bloomBlurShader);
		if (blurShader)
		{
			SDE_PROF_EVENT("BloomBlur");
			auto blurDirection = blurShader->GetUniformHandle("BlurDirection");
			bool horizontal = false;
			for (int i = 0; i < c_bloomBlurIterations; ++i)
			{
				d.SetUniformValue(blurDirection, horizontal ? 0.0f : 1.0f);
				const auto& src = (i == 0) ? m_bloomBrightnessBuffer : *m_bloomBlurBuffers[!horizontal];
				auto& dst = *m_bloomBlurBuffers[horizontal];
				m_targetBlitter.TargetToTarget(d, src, dst, *blurShader);
				horizontal = !horizontal;
			}
		}

		// combine bloom with main buffer
		auto combineShader = sm->GetShader(m_bloomCombineShader);
		if (combineShader)
		{
			SDE_PROF_EVENT("BloomCombine");
			d.BindShaderProgram(*combineShader);		// must happen before setting uniforms
			auto sampler = combineShader->GetUniformHandle("LightingTexture");
			if (sampler != -1)
			{
				// force texture unit 1 since 0 is used by blitter
				d.SetSampler(sampler, mainFb->GetColourAttachment(0).GetResidentHandle());
			}
			m_targetBlitter.TargetToTarget(d, *m_bloomBlurBuffers[0], m_bloomBrightnessBuffer, *combineShader);
		}

		// blit main buffer to backbuffer + tonemap
		d.SetDepthState(false, false);
		d.SetBlending(true);
		auto tonemapShader = sm->GetShader(m_tonemapShader);
		auto blitShader = sm->GetShader(m_blitShader);
		if (blitShader && tonemapShader)
		{
			m_targetBlitter.TargetToTarget(d, m_bloomBrightnessBuffer, *mainFb, *tonemapShader);
			m_targetBlitter.TargetToBackbuffer(d, *mainFb, *blitShader, m_windowSize);
		}
	}

	int Renderer::PrepareCulledShadowInstances(InstanceList& visibleInstances)
	{
		SDE_PROF_EVENT();

		std::sort(visibleInstances.m_instances.begin(), visibleInstances.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2){
			return SortKeyLessThan(q1.m_sortKey, q2.m_sortKey);
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
			std::sort(visibleInstances.m_instances.begin(), visibleInstances.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2) {
				return SortKeyLessThan(q1.m_sortKey, q2.m_sortKey);
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
			std::sort(allVisibleInstances.m_instances.begin(), allVisibleInstances.m_instances.end(), [](const MeshInstance& q1, const MeshInstance& q2){
				return SortKeyLessThan(q1.m_sortKey, q2.m_sortKey);
			});
		}
		list.m_instances = allVisibleInstances.m_instances;
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
				m_jobSystem->ProcessJobThisThread();
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