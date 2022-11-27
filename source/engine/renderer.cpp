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
#include "model.h"
#include "material_helpers.h"
#include "job_system.h"
#include "system_manager.h"
#include <algorithm>
#include <map>

namespace Engine
{
	const uint64_t c_maxInstances = 1024 * 256;		// this needs to include all passes/shadowmap updates
	const uint64_t c_maxDrawCalls = c_maxInstances;
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

	struct RenderPerInstanceData {
		glm::mat4 m_transform;
		Renderer::PerInstanceData m_data;
	};

	DefaultTextures g_defaultTextures;
	std::vector<std::string> g_shadowSamplerNames, g_shadowCubeSamplerNames;

	void Renderer::InstanceList::Reserve(size_t count)
	{
		SDE_PROF_EVENT();
		m_transformBounds.reserve(count);
		m_drawData.reserve(count);
		m_perInstanceData.reserve(count);
		m_entries.reserve(count);
	}

	void Renderer::InstanceList::Resize(size_t count)
	{
		SDE_PROF_EVENT();
		m_transformBounds.resize(count);
		m_drawData.resize(count);
		m_perInstanceData.resize(count);
		m_entries.resize(count);
	}

	void Renderer::InstanceList::Reset()
	{
		SDE_PROF_EVENT();
		m_transformBounds.resize(0);
		m_drawData.resize(0);
		m_perInstanceData.resize(0);
		m_entries.resize(0);
	}

	Renderer::Renderer(JobSystem* js, glm::ivec2 windowSize)
		: m_jobSystem(js)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
		, m_mainFramebufferResolved(windowSize)
		, m_bloomBrightnessBuffer(windowSize)
	{
		SDE_PROF_EVENT();
		m_modelManager = GetSystem<ModelManager>("Models");
		m_textureManager = Engine::GetSystem<TextureManager>("Textures");
		m_shaderManager = Engine::GetSystem<ShaderManager>("Shaders");
		g_defaultTextures["DiffuseTexture"] = m_textureManager->LoadTexture("white.bmp");
		g_defaultTextures["NormalsTexture"] = m_textureManager->LoadTexture("default_normalmap.png");
		g_defaultTextures["SpecularTexture"] = m_textureManager->LoadTexture("white.bmp");

		m_blitShader = m_shaderManager->LoadShader("Basic Blit", "basic_blit.vs", "basic_blit.fs");
		m_bloomBrightnessShader = m_shaderManager->LoadShader("BloomBrightness", "basic_blit.vs", "bloom_brightness.fs");
		m_bloomBlurShader = m_shaderManager->LoadShader("BloomBlur", "basic_blit.vs", "bloom_blur.fs");
		m_bloomCombineShader = m_shaderManager->LoadShader("BloomCombine", "basic_blit.vs", "bloom_combine.fs");
		m_tonemapShader = m_shaderManager->LoadShader("Tonemap", "basic_blit.vs", "tonemap.fs");
		{
			SDE_PROF_EVENT("Create Buffers");
			m_perInstanceData.Create(c_maxInstances * sizeof(RenderPerInstanceData), Render::RenderBufferModification::Dynamic, true);
			m_globalsUniformBuffer.Create(sizeof(GlobalUniforms), Render::RenderBufferModification::Dynamic, true);
			m_drawIndirectBuffer.Create(c_maxDrawCalls * sizeof(Render::DrawIndirectIndexedParams), Render::RenderBufferModification::Dynamic, true);
			m_opaqueInstances.Reserve(c_maxInstances);
			m_transparentInstances.Reserve(c_maxInstances);
			m_allShadowCasterInstances.Reserve(c_maxInstances);
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
		m_opaqueInstances.Reset();
		m_transparentInstances.Reset();
		m_allShadowCasterInstances.Reset();
		m_lights.clear();
		m_nextInstance = 0;
		m_nextDrawCall = 0;
		auto defaultDiffuse = m_textureManager->GetTexture(g_defaultTextures["DiffuseTexture"]);
		m_defaultDiffuseResidentHandle = defaultDiffuse ? defaultDiffuse->GetResidentHandle() : 0;
		auto defaultNormal = m_textureManager->GetTexture(g_defaultTextures["NormalsTexture"]);
		m_defaultNormalResidentHandle = defaultNormal ? defaultNormal->GetResidentHandle() : 0;
		auto defaultSpecular = m_textureManager->GetTexture(g_defaultTextures["SpecularTexture"]);
		m_defaultSpecularResidentHandle = defaultSpecular ? defaultSpecular->GetResidentHandle() : 0;
	}

	void Renderer::SetCamera(const Render::Camera& c)
	{ 
		m_camera = c;
	}

	__m128i ShadowCasterSortKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::MeshChunk* chunks, const Render::Material* meshMat)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));

		const void* chunkPtrVoid = static_cast<const void*>(chunks);
		uintptr_t chunkPtr = reinterpret_cast<uintptr_t>(chunkPtrVoid);
		uint32_t chunkPtrLow = static_cast<uint32_t>((chunkPtr & 0x00000000ffffffff));

		const void* meshMatPtrVoid = static_cast<const void*>(&meshMat);
		uintptr_t meshMatPtr = reinterpret_cast<uintptr_t>(meshMatPtrVoid);
		uint32_t meshMatPtrLow = static_cast<uint32_t>((meshMatPtr & 0x00000000ffffffff));

		__m128i result = _mm_set_epi32(shader.m_index, vaPtrLow, chunkPtrLow, meshMatPtrLow);

		return result;
	}

	__m128i OpaqueSortKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::MeshChunk* chunks, const Render::Material* meshMat, const Render::Material* instanceMat)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));
		
		const void* chunkPtrVoid = static_cast<const void*>(chunks);
		uintptr_t chunkPtr = reinterpret_cast<uintptr_t>(chunkPtrVoid);
		uint32_t chunkPtrLow = static_cast<uint32_t>((chunkPtr & 0x00000000ffffffff));

		const void* meshMatPtrVoid = static_cast<const void*>(&meshMat);
		uintptr_t meshMatPtr = reinterpret_cast<uintptr_t>(meshMatPtrVoid);
		uint32_t meshMatPtrLow = static_cast<uint32_t>((meshMatPtr & 0x00000000ffffffff));

		const void* instPtrVoid = static_cast<const void*>(instanceMat);
		uintptr_t instPtr = reinterpret_cast<uintptr_t>(instPtrVoid);
		uint32_t instPtrLow = static_cast<uint32_t>((instPtr & 0x00000000ffffffff));

		uint32_t materialsCombined = meshMatPtrLow ^ instPtrLow;	// is there a better way?
		
		__m128i result = _mm_set_epi32(shader.m_index, vaPtrLow, chunkPtrLow, materialsCombined);

		return result;
	}

	__m128i TransparentSortKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::MeshChunk* chunks, const Render::Material* instanceMat, float distanceToCamera)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));

		const void* chunkPtrVoid = static_cast<const void*>(chunks);
		uintptr_t chunkPtr = reinterpret_cast<uintptr_t>(chunkPtrVoid);
		uint32_t chunkPtrLow = static_cast<uint32_t>((chunkPtr & 0x00000000ffffffff));
		uint32_t vaChunkCombined = vaPtrLow ^ chunkPtrLow;

		const void* matPtrVoid = static_cast<const void*>(instanceMat);
		uintptr_t matPtr = reinterpret_cast<uintptr_t>(matPtrVoid);
		uint32_t matPtrLow = static_cast<uint32_t>((matPtr & 0x00000000ffffffff));
		
		const void* instPtrVoid = static_cast<const void*>(instanceMat);
		uintptr_t instPtr = reinterpret_cast<uintptr_t>(instPtrVoid);
		uint32_t instPtrLow = static_cast<uint32_t>((instPtr & 0x00000000ffffffff));
		uint32_t materialsCombined = matPtrLow ^ instPtrLow;	// is there a better way?

		uint32_t distanceToCameraAsInt = static_cast<uint32_t>(distanceToCamera * 100.0f);
		__m128i result = _mm_set_epi32(distanceToCameraAsInt, shader.m_index, vaChunkCombined, materialsCombined);

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

	inline bool SortKeyEqual(__m128i a, __m128i b)
	{
		__m128i compared = _mm_cmpeq_epi8(a, b);     
		uint16_t bitMask = _mm_movemask_epi8(compared);
		return bitMask == 0xffff;
	}

	inline void Renderer::SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
		const struct ShaderHandle& shader, const Render::VertexArray* va, const Render::RenderBuffer* ib,
		const Render::MeshChunk* chunks, uint32_t chunkCount, const PerInstanceData& pid)
	{
		const auto foundShader = m_shaderManager->GetShader(shader);
		const uint64_t dataIndex = list.m_entries.size();
		list.m_transformBounds.emplace_back(InstanceList::TransformBounds{trns, aabbMin, aabbMax});
		list.m_drawData.push_back({ foundShader, va, ib, nullptr, nullptr, chunks, chunkCount});
		list.m_perInstanceData.emplace_back(pid);
		list.m_entries.emplace_back(InstanceList::Entry{sortKey, dataIndex});
	}

	inline void Renderer::SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, 
		const Render::VertexArray* va, const Render::RenderBuffer* ib, const Render::MeshChunk* chunks, uint32_t chunkCount, const Render::Material* meshMaterial,
		const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat)
	{
		const auto foundShader = m_shaderManager->GetShader(shader);
		const uint64_t dataIndex = list.m_entries.size();
		list.m_transformBounds.emplace_back(InstanceList::TransformBounds{ trns, aabbMin, aabbMax });
		list.m_drawData.emplace_back(InstanceList::DrawData{ foundShader, va, ib, meshMaterial, instanceMat, chunks, chunkCount });
		list.m_perInstanceData.emplace_back(PerInstanceData{});
		list.m_entries.emplace_back(InstanceList::Entry{ sortKey, dataIndex });
	}

	void Renderer::SubmitInstance(InstanceList& list, __m128i sortKey, const glm::mat4& trns, const Render::Mesh& mesh, const struct ShaderHandle& shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax, const Render::Material* instanceMat)
	{
		SubmitInstance(list, sortKey, trns, 
			&mesh.GetVertexArray(), mesh.GetIndexBuffer().get(), &mesh.GetChunks()[0], mesh.GetChunks().size(), &mesh.GetMaterial(),
			shader, aabbMin, aabbMax, instanceMat);
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, glm::vec3 boundsMin, glm::vec3 boundsMax, const Render::Material* instanceMat)
	{
		bool castShadow = mesh.GetMaterial().GetCastsShadows();
		bool isTransparent = mesh.GetMaterial().GetIsTransparent();

		if (instanceMat != nullptr)
		{
			castShadow &= instanceMat->GetCastsShadows();
			isTransparent |= instanceMat->GetIsTransparent();
		}

		if (castShadow)
		{
			auto shadowShader = m_shaderManager->GetShadowsShader(shader);
			if (shadowShader.m_index != (uint32_t)-1)
			{
				auto sortKey = ShadowCasterSortKey(shadowShader, &mesh.GetVertexArray(), mesh.GetChunks().data(), &mesh.GetMaterial());
				SubmitInstance(m_allShadowCasterInstances, sortKey, transform, mesh, shadowShader, boundsMin, boundsMax);
			}
		}

		if (isTransparent)
		{
			const float distanceToCamera = glm::length(glm::vec3(transform[3]) - m_camera.Position());
			__m128i sortKey = TransparentSortKey(shader, &mesh.GetVertexArray(), mesh.GetChunks().data(), instanceMat, distanceToCamera);
			SubmitInstance(m_transparentInstances, sortKey, transform, mesh, shader, boundsMin, boundsMax, instanceMat);
		}
		else
		{
			__m128i sortKey = OpaqueSortKey(shader, &mesh.GetVertexArray(), mesh.GetChunks().data(), &mesh.GetMaterial(), instanceMat);
			SubmitInstance(m_opaqueInstances, sortKey, transform, mesh, shader, boundsMin, boundsMax, instanceMat);
		}
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, const Render::Material* instanceMat)
	{
		SubmitInstance(transform, mesh, shader, { -FLT_MAX, -FLT_MAX, -FLT_MAX }, { FLT_MAX, FLT_MAX, FLT_MAX }, instanceMat);
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader, const Model::MeshPart::DrawData* partOverride, uint32_t overrideCount)
	{
		const auto theModel = m_modelManager->GetModel(model);
		const auto theShader = m_shaderManager->GetShader(shader);
		if (theModel != nullptr && theShader != nullptr)
		{
			ShaderHandle shadowShader = m_shaderManager->GetShadowsShader(shader);
			const Render::VertexArray* va = m_modelManager->GetVertexArray();
			const Render::RenderBuffer* ib = m_modelManager->GetIndexBuffer();
			const int partCount = theModel->MeshParts().size();
			for (int partIndex = 0; partIndex < partCount; ++partIndex)
			{
				const auto& meshPart = theModel->MeshParts()[partIndex];
				const Model::MeshPart::DrawData& partDrawData = overrideCount > partIndex ? partOverride[partIndex] : meshPart.m_drawData;
				auto diffuse = m_textureManager->GetTexture(partDrawData.m_diffuseTexture);
				auto normal = m_textureManager->GetTexture(partDrawData.m_normalsTexture);
				auto specular = m_textureManager->GetTexture(partDrawData.m_specularTexture);

				PerInstanceData pid;	// it may be better to defer getting the texture handles until post-culling?
				pid.m_diffuseOpacity = partDrawData.m_diffuseOpacity;
				pid.m_specular = partDrawData.m_specular;
				pid.m_shininess = partDrawData.m_shininess;
				pid.m_diffuseTexture = diffuse ? diffuse->GetResidentHandle() : m_defaultDiffuseResidentHandle;
				pid.m_normalsTexture = normal ? normal->GetResidentHandle() : m_defaultNormalResidentHandle;
				pid.m_specularTexture = specular ? specular->GetResidentHandle() : m_defaultSpecularResidentHandle;

				const glm::mat4 instanceTransform = transform * meshPart.m_transform;
				const glm::vec3 boundsMin = meshPart.m_boundsMin, boundsMax = meshPart.m_boundsMax;
				if (partDrawData.m_castsShadows && shadowShader.m_index != (uint32_t)-1)
				{
					auto sortKey = ShadowCasterSortKey(shadowShader, va, meshPart.m_chunks.data(), nullptr);
					SubmitInstance(m_allShadowCasterInstances, sortKey, instanceTransform, boundsMin, boundsMax,
						shadowShader, va, ib, &meshPart.m_chunks[0], (uint32_t)meshPart.m_chunks.size(), pid);
				}

				if (partDrawData.m_isTransparent || pid.m_diffuseOpacity.a != 1.0f)
				{
					const float distanceToCamera = glm::length(glm::vec3(instanceTransform[3]) - m_camera.Position());
					auto sortKey = TransparentSortKey(shader, va, meshPart.m_chunks.data(), nullptr, distanceToCamera);
					SubmitInstance(m_transparentInstances, sortKey, instanceTransform, boundsMin, boundsMax,
						shader, va, ib, &meshPart.m_chunks[0], (uint32_t)meshPart.m_chunks.size(), pid);
				}
				else
				{
					auto opaqueSortKey = OpaqueSortKey(shader, va, meshPart.m_chunks.data(), nullptr, nullptr);
					SubmitInstance(m_opaqueInstances, opaqueSortKey, instanceTransform, boundsMin, boundsMax,
						shader, va, ib, &meshPart.m_chunks[0], (uint32_t)meshPart.m_chunks.size(), pid);
				}
			}
		}
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader)
	{
		SubmitInstance(transform, model, shader, nullptr, 0);
	}

	void Renderer::SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 colour, float ambientStr, float distance, float attenuation, glm::vec2 spotAngles,
		Render::FrameBuffer* sm, float shadowBias, glm::mat4 shadowMatrix)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = { position, 2.0f };	// spotlight
		newLight.m_direction = direction;
		newLight.m_maxDistance = distance;
		newLight.m_attenuationCompress = attenuation;
		newLight.m_shadowMap = sm;
		newLight.m_lightspaceMatrix = shadowMatrix;
		newLight.m_shadowBias = shadowBias;
		newLight.m_updateShadowmap = sm != nullptr;
		newLight.m_spotlightAngles = spotAngles;
		m_lights.push_back(newLight);
	}

	void Renderer::DirectionalLight(glm::vec3 direction, glm::vec3 colour, float ambientStr, Render::FrameBuffer* shadowMap, float shadowBias, glm::mat4 shadowMatrix)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = { 0.0f,0.0f,0.0f,0.0f };	// directional
		newLight.m_direction = direction;
		newLight.m_maxDistance = 0.0f;
		newLight.m_attenuationCompress = 0.0f;
		newLight.m_shadowMap = shadowMap;
		newLight.m_lightspaceMatrix = shadowMatrix;
		newLight.m_shadowBias = shadowBias;
		newLight.m_updateShadowmap = shadowMap != nullptr;
		m_lights.push_back(newLight);
	}

	void Renderer::PointLight(glm::vec3 position, glm::vec3 colour, float ambientStr, float distance, float attenuation,
		Render::FrameBuffer* shadowMap, float shadowBias, glm::mat4 shadowMatrix)
	{
		Light newLight;
		newLight.m_colour = glm::vec4(colour, ambientStr);
		newLight.m_position = { position,1.0f };	// point light
		newLight.m_maxDistance = distance;
		newLight.m_attenuationCompress = attenuation;
		newLight.m_shadowMap = shadowMap;
		newLight.m_lightspaceMatrix = shadowMatrix;
		newLight.m_shadowBias = shadowBias;
		newLight.m_updateShadowmap = shadowMap != nullptr;
		m_lights.push_back(newLight);
	}

	int Renderer::PopulateInstanceBuffers(InstanceList& list, const EntryList& entries)
	{
		SDE_PROF_EVENT();
		static std::vector<RenderPerInstanceData> instanceData;
		{
			SDE_PROF_EVENT("Reserve");
			instanceData.reserve(entries.size());
			instanceData.resize(0);
		}

		{
			SDE_PROF_EVENT("Copy");
			for (int e = 0; e < entries.size(); ++e)
			{
				const auto index = entries[e].m_dataIndex;
				instanceData.emplace_back(RenderPerInstanceData{ list.m_transformBounds[index].m_transform, list.m_perInstanceData[index] });
			}
		}

		// copy the instance buffers to gpu
		int instanceIndex = -1;
		if (instanceData.size() > 0 && m_nextInstance + instanceData.size() < c_maxInstances)
		{
			m_perInstanceData.SetData(m_nextInstance * sizeof(RenderPerInstanceData), instanceData.size() * sizeof(RenderPerInstanceData), instanceData.data());
			instanceIndex = m_nextInstance;
			m_nextInstance += instanceData.size();
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

	void Renderer::PrepareDrawBuckets(const InstanceList& list, const EntryList& entries, int baseIndex, std::vector<DrawBucket>& buckets)
	{
		SDE_PROF_EVENT();
		auto firstInstance = entries.begin();
		auto finalInstance = entries.end();
		static std::vector<Render::DrawIndirectIndexedParams> tmpDrawList;
		tmpDrawList.reserve(c_maxInstances);
		tmpDrawList.resize(0);
		int drawUploadIndex = m_nextDrawCall;	// upload the entire buffer in one
		while (firstInstance != finalInstance)
		{
			const auto& drawData = list.m_drawData[firstInstance->m_dataIndex];

			// Batch by shader, va/ib, primitive type and material (essentially any kind of 'pipeline' change)
			// This only works if the lists are correctly sorted!
			auto lastMeshInstance = std::find_if(firstInstance, finalInstance, 
				[firstInstance, &drawData, &list](const InstanceList::Entry& e) -> bool {
				const auto& thisDrawData = list.m_drawData[e.m_dataIndex];
				return  thisDrawData.m_va != drawData.m_va ||
					thisDrawData.m_ib != drawData.m_ib ||
					thisDrawData.m_chunks[0].m_primitiveType != drawData.m_chunks[0].m_primitiveType ||
					thisDrawData.m_shader != drawData.m_shader ||
					thisDrawData.m_meshMaterial != drawData.m_meshMaterial ||
					thisDrawData.m_instanceMaterial != drawData.m_instanceMaterial;
			});
			const auto instanceCount = (uint32_t)(lastMeshInstance - firstInstance);
			if (instanceCount == 0)
			{
				continue;
			}
			DrawBucket newBucket;
			newBucket.m_shader = drawData.m_shader;
			newBucket.m_va = drawData.m_va;
			newBucket.m_ib = drawData.m_ib;
			newBucket.m_meshMaterial = drawData.m_meshMaterial;
			newBucket.m_instanceMaterial = drawData.m_meshMaterial;
			newBucket.m_primitiveType = (uint32_t)drawData.m_chunks[0].m_primitiveType;	// danger, no error checks here!
			newBucket.m_drawCount = 0;
			newBucket.m_firstDrawIndex = m_nextDrawCall;
			
			// now find all chunks that can go in this bucket
			const uint32_t firstInstanceIndex = (uint32_t)(firstInstance - entries.begin());
			if (drawData.m_ib != nullptr)
			{
				int drawCallCount = 0;
				for (auto drawStart = firstInstance; drawStart != lastMeshInstance; ++drawStart)
				{
					const auto& thisDrawData = list.m_drawData[drawStart->m_dataIndex];
					for (int c = 0; c < thisDrawData.m_chunkCount; ++c)
					{
						Render::DrawIndirectIndexedParams ip;
						ip.m_indexCount = thisDrawData.m_chunks[c].m_vertexCount;
						ip.m_instanceCount = 1;		// todo - we can (and should) do chunk instancing
						ip.m_firstIndex = thisDrawData.m_chunks[c].m_firstVertex;
						ip.m_baseVertex = 0;
						ip.m_baseInstance = firstInstanceIndex + baseIndex + drawCallCount;
						tmpDrawList.emplace_back(ip);
						++drawCallCount;
					}
				}
				newBucket.m_drawCount = drawCallCount;
			}
			else
			{
				assert("Todo - nonindexed stuff");
			}
			buckets.emplace_back(newBucket);
			assert(m_nextDrawCall + newBucket.m_drawCount < c_maxDrawCalls);
			m_nextDrawCall += newBucket.m_drawCount;
			firstInstance = lastMeshInstance;
		}
		if (tmpDrawList.size() > 0 && drawUploadIndex + tmpDrawList.size() < m_nextDrawCall + c_maxInstances)
		{
			SDE_PROF_EVENT("Upload draw calls");
			m_drawIndirectBuffer.SetData(drawUploadIndex * sizeof(Render::DrawIndirectIndexedParams),
				tmpDrawList.size() * sizeof(Render::DrawIndirectIndexedParams), tmpDrawList.data());
		}
	}

	void Renderer::DrawBuckets(Render::Device& d, const std::vector<DrawBucket>& buckets, bool bindShadowmaps, Render::UniformBuffer* uniforms)
	{
		SDE_PROF_EVENT();
		const Render::ShaderProgram* lastShaderUsed = nullptr;
		const Render::VertexArray* lastVAUsed = nullptr;
		const Render::RenderBuffer* lastIBUsed = nullptr;
		const Render::Material* lastMeshMaterial = nullptr;
		const Render::Material* lastInstanceMaterial = nullptr;
		for (const auto& b : buckets)
		{
			m_frameStats.m_batchesDrawn++;
			if (b.m_shader != lastShaderUsed)
			{
				m_frameStats.m_shaderBinds++;
				d.BindShaderProgram(*b.m_shader);
				d.BindUniformBufferIndex(*b.m_shader, "Globals", 0);
				d.SetUniforms(*b.m_shader, m_globalsUniformBuffer, 0);
				d.BindStorageBuffer(0, m_perInstanceData);		// bind instancing data once per shader
				d.BindDrawIndirectBuffer(m_drawIndirectBuffer);
				if (uniforms != nullptr)
				{
					uniforms->Apply(d, *b.m_shader);
				}
				if (bindShadowmaps)
				{
					BindShadowmaps(d, *b.m_shader, 0);
				}
				lastShaderUsed = b.m_shader;
			}
			if (b.m_meshMaterial != nullptr && b.m_meshMaterial != lastMeshMaterial)
			{
				ApplyMaterial(d, *b.m_shader, *b.m_meshMaterial, &g_defaultTextures);
			}
			if (b.m_instanceMaterial != nullptr && b.m_instanceMaterial != lastInstanceMaterial)
			{
				ApplyMaterial(d, *b.m_shader, *b.m_instanceMaterial, &g_defaultTextures);
				lastInstanceMaterial = b.m_instanceMaterial;
			}
			if (b.m_va != lastVAUsed)
			{
				m_frameStats.m_vertexArrayBinds++;
				d.BindVertexArray(*b.m_va);
				lastVAUsed = b.m_va;
			}
			if (b.m_ib != nullptr)
			{
				if (b.m_ib != lastIBUsed)
				{
					d.BindIndexBuffer(*b.m_ib);
					lastIBUsed = b.m_ib;
				}
			}
			m_frameStats.m_drawCalls++;
			d.DrawPrimitivesIndirectIndexed((Render::PrimitiveType)b.m_primitiveType, b.m_firstDrawIndex, b.m_drawCount);
		}
	}

	void Renderer::DrawInstances(Render::Device& d, const InstanceList& list, const EntryList& entries, int baseIndex, bool bindShadowmaps, Render::UniformBuffer* uniforms)
	{
		SDE_PROF_EVENT();
		auto firstInstance = entries.begin();
		auto finalInstance = entries.end();
		const Render::ShaderProgram* lastShaderUsed = nullptr;	// avoid setting the same shader
		const Render::Material* lastInstanceMaterial = nullptr;	// avoid setting instance materials for the same mesh/shaders
		const Render::VertexArray* lastVAUsed = nullptr;
		const Render::RenderBuffer* lastIBUsed = nullptr;
		while (firstInstance != finalInstance)
		{
			const auto& drawData = list.m_drawData[firstInstance->m_dataIndex];

			// Batch by shader, va/ib, chunks and materials
			auto lastMeshInstance = std::find_if(firstInstance, finalInstance, [firstInstance, &drawData, &list](const InstanceList::Entry& m) -> bool {
				const auto& thisDrawData = list.m_drawData[m.m_dataIndex];
				return  thisDrawData.m_va != drawData.m_va ||
					thisDrawData.m_ib != drawData.m_ib ||
					thisDrawData.m_chunks != drawData.m_chunks ||
					thisDrawData.m_chunks[0].m_primitiveType != drawData.m_chunks[0].m_primitiveType ||
					thisDrawData.m_shader != drawData.m_shader ||
					thisDrawData.m_meshMaterial != drawData.m_meshMaterial ||
					thisDrawData.m_instanceMaterial != drawData.m_instanceMaterial;
			});
			auto instanceCount = (uint32_t)(lastMeshInstance - firstInstance);
			Render::ShaderProgram* theShader = drawData.m_shader;
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
					d.BindStorageBuffer(0, m_perInstanceData);		// bind instancing data once per shader
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
					lastInstanceMaterial = nullptr;
				}

				// apply mesh material uniforms and samplers
				if (drawData.m_meshMaterial != nullptr)
				{
					ApplyMaterial(d, *theShader, *drawData.m_meshMaterial, &g_defaultTextures);
				}

				// apply instance material uniforms and samplers (materials can be shared across instances!)
				auto instanceMaterial = drawData.m_instanceMaterial;
				if (instanceMaterial != nullptr && instanceMaterial != lastInstanceMaterial)
				{
					ApplyMaterial(d, *theShader, *instanceMaterial, &g_defaultTextures);
					lastInstanceMaterial = instanceMaterial;
				}

				if (drawData.m_va != lastVAUsed)
				{
					m_frameStats.m_vertexArrayBinds++;
					d.BindVertexArray(*drawData.m_va);
					lastVAUsed = drawData.m_va;
				}

				// draw the chunks
				if (drawData.m_ib != nullptr)
				{
					if (drawData.m_ib != lastIBUsed)
					{
						d.BindIndexBuffer(*drawData.m_ib);
						lastIBUsed = drawData.m_ib;
					}
					
					for(uint32_t c=0; c< drawData.m_chunkCount;++c)
					{
						const auto& chunk = drawData.m_chunks[c];
						uint32_t firstIndex = (uint32_t)(firstInstance - entries.begin());
						d.DrawPrimitivesInstancedIndexed(chunk.m_primitiveType,	chunk.m_firstVertex, chunk.m_vertexCount,
							instanceCount, firstIndex + baseIndex);
						m_frameStats.m_drawCalls++;
						m_frameStats.m_totalVertices += (uint64_t)chunk.m_vertexCount * instanceCount;
					}
				}
				else
				{
					for (uint32_t c = 0; c < drawData.m_chunkCount; ++c)
					{
						const auto& chunk = drawData.m_chunks[c];
						uint32_t firstIndex = (uint32_t)(firstInstance - entries.begin());
						d.DrawPrimitivesInstanced(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount, instanceCount, firstIndex + baseIndex);
						m_frameStats.m_drawCalls++;
						m_frameStats.m_totalVertices += (uint64_t)chunk.m_vertexCount * instanceCount;
					}
				}
			}
			firstInstance = lastMeshInstance;
		}
	}

	void Renderer::FindVisibleInstancesAsync(const Frustum& f, const InstanceList& src, EntryList& result, OnFindVisibleComplete onComplete)
	{
		SDE_PROF_EVENT();

		auto jobs = Engine::GetSystem<JobSystem>("Jobs");
		const uint32_t partsPerJob = 2000;
		const uint32_t jobCount = src.m_entries.size() < partsPerJob ? 1 : src.m_entries.size() / partsPerJob;
		if (src.m_entries.size() == 0)
		{
			onComplete(0);
			return;
		}
		struct JobData
		{
			std::atomic<uint32_t> m_jobsRemaining = 0;
			std::atomic<uint32_t> m_count = 0;
		};
		JobData* jobData = new JobData();
		if(result.size() < src.m_entries.size())
		{
			SDE_PROF_EVENT("Resize results array");
			result.resize(src.m_entries.size());
		}
		jobData->m_jobsRemaining = ceilf((float)src.m_entries.size() / partsPerJob);
		for (uint32_t p = 0; p < src.m_entries.size(); p += partsPerJob)
		{
			auto cullPartsJob = [jobData, &src, &result, p, partsPerJob, f, onComplete](void*) {
				SDE_PROF_EVENT("Cull instances");
				uint32_t firstIndex = p;
				uint32_t lastIndex = std::min((uint32_t)src.m_entries.size(), firstIndex + partsPerJob);
				EntryList localResults;	// collect results for this job
				localResults.reserve(partsPerJob);
				for (uint32_t id = firstIndex; id < lastIndex; ++id)
				{
					const auto& theTransformBounds = src.m_transformBounds[src.m_entries[id].m_dataIndex];
					if (f.IsBoxVisible(theTransformBounds.m_aabbMin, theTransformBounds.m_aabbMax, theTransformBounds.m_transform))
					{
						localResults.emplace_back(src.m_entries[id]);
					}
				}
				if (localResults.size() > 0)	// copy the results to the main list
				{
					SDE_PROF_EVENT("Push results");
					uint64_t offset = jobData->m_count.fetch_add(localResults.size());
					for (int r = 0; r < localResults.size(); ++r)
					{
						result[offset + r] = localResults[r];
					}
				}
				if (jobData->m_jobsRemaining.fetch_sub(1) == 1)		// this was the last culling job, time to sort!
				{
					SDE_PROF_EVENT("SortResults");
					{
						std::sort(result.begin(), result.begin() + jobData->m_count,
							[](const InstanceList::Entry& s1, const InstanceList::Entry& s2) {
								if (SortKeyLessThan(s1.m_sortKey, s2.m_sortKey))
								{
									return true;
								}
								else if (SortKeyEqual(s1.m_sortKey, s2.m_sortKey))
								{
									return s1.m_dataIndex < s2.m_dataIndex;
								}
								else
								{
									return false;
								}
							});
					}
					result.resize(jobData->m_count);
					onComplete(jobData->m_count);
					delete jobData;
				}
			};
			jobs->PushJob(cullPartsJob);
		}
	}

	int Renderer::RenderShadowmap(Render::Device& d, Light& l, const std::vector<std::unique_ptr<EntryList>>& visibleInstances, int instanceListStartIndex)
	{
		SDE_PROF_EVENT();
		m_frameStats.m_shadowMapUpdates++;
		if (l.m_position.w == 0.0f || l.m_position.w == 2.0f)		// directional / spot
		{
			const auto& instances = *visibleInstances[instanceListStartIndex];
			int baseIndex = PopulateInstanceBuffers(m_allShadowCasterInstances, instances);
			Render::UniformBuffer lightMatUniforms;
			lightMatUniforms.SetValue("ShadowLightSpaceMatrix", l.m_lightspaceMatrix);
			lightMatUniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
			d.DrawToFramebuffer(*l.m_shadowMap);
			d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
			d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);
			d.SetScissorEnabled(false);
			if (m_useDrawIndirect)
			{
				static std::vector<DrawBucket> drawBuckets;
				drawBuckets.reserve(2000);
				drawBuckets.resize(0);
				PrepareDrawBuckets(m_allShadowCasterInstances, instances, baseIndex, drawBuckets);
				DrawBuckets(d, drawBuckets, false, &lightMatUniforms);
			}
			else
			{
				DrawInstances(d, m_allShadowCasterInstances, instances, baseIndex, false, &lightMatUniforms);
			}
			m_frameStats.m_totalShadowInstances += m_allShadowCasterInstances.m_entries.size();
			m_frameStats.m_renderedShadowInstances += instances.size();
			return instanceListStartIndex + 1;
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
			Frustum shadowFrustums[6];
			for (uint32_t cubeFace = 0; cubeFace < 6; ++cubeFace)
			{
				auto& instances = *visibleInstances[instanceListStartIndex + cubeFace];
				int baseIndex = PopulateInstanceBuffers(m_allShadowCasterInstances, instances);
				d.DrawToFramebuffer(*l.m_shadowMap, cubeFace);
				d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
				d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
				d.SetBackfaceCulling(true, true);	// backface culling, ccw order
				d.SetBlending(false);
				d.SetScissorEnabled(false);
				uniforms.SetValue("ShadowLightSpaceMatrix", shadowTransforms[cubeFace]);
				uniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
				if (m_useDrawIndirect)
				{
					static std::vector<DrawBucket> drawBuckets;
					drawBuckets.reserve(2000);
					drawBuckets.resize(0);
					PrepareDrawBuckets(m_allShadowCasterInstances, instances, baseIndex, drawBuckets);
					DrawBuckets(d, drawBuckets, false, &uniforms);
				}
				else
				{
					DrawInstances(d, m_allShadowCasterInstances, instances, baseIndex, false, &uniforms);
				}
				m_frameStats.m_totalShadowInstances += m_allShadowCasterInstances.m_entries.size();
				m_frameStats.m_renderedShadowInstances += instances.size();
			}
			return instanceListStartIndex + 6;
		}
	}

	void Renderer::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();
		m_frameStats = {};
		m_frameStats.m_instancesSubmitted = m_opaqueInstances.m_entries.size() + m_transparentInstances.m_entries.size();
		m_frameStats.m_activeLights = std::min(m_lights.size(), c_maxLights);

		CullLights();

		static EntryList visibleOpaques, visibleTransparents;
		static std::vector<std::unique_ptr<EntryList>> visibleShadowCasters;
		std::atomic<bool> opaquesReady = false, transparentsReady = false;
		std::atomic<int> shadowsInFlight = 0;
		{
			SDE_PROF_EVENT("BeginCulling");

			// Kick off the shadow caster culling first, we need the results asap!
			int shadowCasterListId = 0;	// tracks the current result list to write to
			for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
			{
				if (m_lights[l].m_shadowMap != nullptr && m_lights[l].m_updateShadowmap)
				{
					while(visibleShadowCasters.size() < shadowCasterListId + 1)
					{
						auto newInstanceList = std::make_unique<EntryList>();
						newInstanceList->reserve(m_allShadowCasterInstances.m_entries.size());
						visibleShadowCasters.emplace_back(std::move(newInstanceList));
					}
					if (m_lights[l].m_position.w == 0.0f || m_lights[l].m_position.w == 2.0f)		// directional / spot
					{
						++shadowsInFlight;
						Frustum shadowFrustum(m_lights[l].m_lightspaceMatrix);
						visibleShadowCasters[shadowCasterListId]->resize(0);
						FindVisibleInstancesAsync(shadowFrustum, m_allShadowCasterInstances,
							*visibleShadowCasters[shadowCasterListId], [shadowCasterListId, &shadowsInFlight](size_t count) {
								--shadowsInFlight;
						});
						shadowCasterListId++;
					}
					else
					{
						while (visibleShadowCasters.size() < shadowCasterListId + 6)
						{
							auto newInstanceList = std::make_unique<EntryList>();
							newInstanceList->reserve(m_allShadowCasterInstances.m_entries.size());
							visibleShadowCasters.emplace_back(std::move(newInstanceList));
						}
						const auto pos3 = glm::vec3(m_lights[l].m_position);
						const glm::mat4 shadowTransforms[] = {
							m_lights[l].m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
							m_lights[l].m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)),
							m_lights[l].m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),
							m_lights[l].m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)),
							m_lights[l].m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)),
							m_lights[l].m_lightspaceMatrix * glm::lookAt(pos3, pos3 + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0,0.0f))
						};
						for (int cubeSide = 0; cubeSide < 6; ++cubeSide)
						{
							Frustum shadowFrustum(shadowTransforms[cubeSide]);
							++shadowsInFlight;
							FindVisibleInstancesAsync(shadowFrustum, m_allShadowCasterInstances,
								*visibleShadowCasters[shadowCasterListId], [shadowCasterListId, &shadowsInFlight](size_t count) {
									--shadowsInFlight;
								});
							shadowCasterListId++;
						}
					}
				}
			}

			visibleOpaques.reserve(m_opaqueInstances.m_entries.size());
			visibleTransparents.reserve(m_transparentInstances.m_entries.size());
			Frustum viewFrustum(m_camera.ProjectionMatrix() * m_camera.ViewMatrix());
			visibleOpaques.resize(0);
			FindVisibleInstancesAsync(viewFrustum, m_opaqueInstances, visibleOpaques, [&](size_t resultCount) {
				opaquesReady = true;
			});
			visibleTransparents.resize(0);
			FindVisibleInstancesAsync(viewFrustum, m_transparentInstances, visibleTransparents, [&](size_t resultCount) {
				transparentsReady = true;
			});
		}

		{
			SDE_PROF_EVENT("Clear main framebuffer");
			d.SetDepthState(true, true);	// make sure depth write is enabled before clearing!
			d.ClearFramebufferColourDepth(m_mainFramebuffer, m_clearColour, FLT_MAX);
		}

		// setup global constants
		const auto projectionMat = m_camera.ProjectionMatrix();
		const auto viewMat = m_camera.ViewMatrix();
		UpdateGlobals(projectionMat, viewMat);

		{
			SDE_PROF_EVENT("Wait for shadow lists");
			while (shadowsInFlight > 0)
			{
				Core::Thread::Sleep(0);
			}
		}
		int startIndex = 0;
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr && m_lights[l].m_updateShadowmap)
			{
				startIndex = RenderShadowmap(d, m_lights[l], visibleShadowCasters, startIndex);
			}
		}

		// lighting to main frame buffer
		{
			SDE_PROF_EVENT("RenderMainBuffer");

			// render opaques
			d.SetWireframeDrawing(m_showWireframe);
			d.DrawToFramebuffer(m_mainFramebuffer);
			d.SetViewport(glm::ivec2(0, 0), m_mainFramebuffer.Dimensions());
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);				// no blending for opaques
			d.SetScissorEnabled(false);			// (don't) scissor me timbers

			{
				SDE_PROF_EVENT("Wait for opaques");
				while (!opaquesReady)
				{
					m_jobSystem->ProcessJobThisThread();
				}
			}
			m_frameStats.m_totalOpaqueInstances = m_opaqueInstances.m_entries.size();
			int baseInstance = PopulateInstanceBuffers(m_opaqueInstances, visibleOpaques);
			if (m_useDrawIndirect)
			{
				static std::vector<DrawBucket> drawBuckets;
				drawBuckets.reserve(2000);
				drawBuckets.resize(0);	
				PrepareDrawBuckets(m_opaqueInstances, visibleOpaques, baseInstance, drawBuckets);
				DrawBuckets(d, drawBuckets, true, nullptr);
			}
			else
			{
				DrawInstances(d, m_opaqueInstances, visibleOpaques, baseInstance, true, nullptr);
			}
			m_frameStats.m_renderedOpaqueInstances = visibleOpaques.size();

			// render transparents
			d.SetDepthState(true, false);		// enable z-test, disable write
			d.SetBlending(true);
			{
				SDE_PROF_EVENT("Wait for transparents");
				while (!transparentsReady)
				{
					m_jobSystem->ProcessJobThisThread();
				}
			}
			m_frameStats.m_totalTransparentInstances = m_transparentInstances.m_entries.size();
			baseInstance = PopulateInstanceBuffers(m_transparentInstances, visibleTransparents);
			if (m_useDrawIndirect)
			{
				static std::vector<DrawBucket> drawBuckets;
				drawBuckets.reserve(2000);
				drawBuckets.resize(0);
				PrepareDrawBuckets(m_transparentInstances, visibleTransparents, baseInstance, drawBuckets);
				DrawBuckets(d, drawBuckets, true, nullptr);
			}
			else
			{
				DrawInstances(d, m_transparentInstances, visibleTransparents, baseInstance, true, nullptr);
			}
			m_frameStats.m_renderedTransparentInstances = visibleTransparents.size();

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
		auto brightnessShader = m_shaderManager->GetShader(m_bloomBrightnessShader);
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
		auto blurShader = m_shaderManager->GetShader(m_bloomBlurShader);
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
		auto combineShader = m_shaderManager->GetShader(m_bloomCombineShader);
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
		auto tonemapShader = m_shaderManager->GetShader(m_tonemapShader);
		auto blitShader = m_shaderManager->GetShader(m_blitShader);
		if (blitShader && tonemapShader)
		{
			m_targetBlitter.TargetToTarget(d, m_bloomBrightnessBuffer, *mainFb, *tonemapShader);
			m_targetBlitter.TargetToBackbuffer(d, *mainFb, *blitShader, m_windowSize);
		}
	}
}