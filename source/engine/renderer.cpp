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
#include "model.h"
#include "material_helpers.h"
#include "job_system.h"
#include "system_manager.h"
#include "ssao.h"
#include "2d_render_context.h"
#include <algorithm>
#include <map>

namespace Engine
{
	const uint64_t c_maxInstances = 1024 * 384;		// this needs to include all passes/shadowmap updates
	const uint64_t c_maxDrawCalls = c_maxInstances;
	const uint64_t c_maxLights = 4096;
	const uint32_t c_maxShadowMaps = 16;
	const int32_t c_bloomBufferSizeDivider = 2;	// windowsize/divider
	const uint32_t c_bloomBlurIterations = 10;

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
		float m_hdrExposure;
		glm::vec2 m_windowDims;
		glm::ivec2 m_lightTileCounts;
	};

	struct RenderPerInstanceData {
		glm::mat4 m_transform;
		PerInstanceData m_data;
	};

	DefaultTextures g_defaultTextures;
	std::vector<std::string> g_shadowSamplerNames, g_shadowCubeSamplerNames;

	Renderer::Renderer(JobSystem* js, glm::ivec2 windowSize)
		: m_jobSystem(js)
		, m_windowSize(windowSize)
		, m_mainFramebuffer(windowSize)
		, m_mainFramebufferResolved(windowSize)
		, m_gBuffer(windowSize)
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
		m_deferredLightingShader = m_shaderManager->LoadShader("DeferredLighting", "basic_blit.vs", "deferredlight.fs");
		{
			SDE_PROF_EVENT("Create Buffers");
			const uint32_t maxLightTileData = (c_maxLightsPerTile + 1) * m_lightTileCounts.x * m_lightTileCounts.y;
			for (int i = 0; i < c_maxFramesAhead; ++i)
			{
				m_perInstanceData[i].Create(c_maxInstances * sizeof(RenderPerInstanceData), Render::RenderBufferModification::Dynamic, true);
				m_globalsUniformBuffer[i].Create(sizeof(GlobalUniforms), Render::RenderBufferModification::Dynamic, true);
				m_drawIndirectBuffer[i].Create(c_maxDrawCalls * sizeof(Render::DrawIndirectIndexedParams), Render::RenderBufferModification::Dynamic, true);
				m_lightTileData[i].Create(sizeof(uint32_t) * maxLightTileData, Render::RenderBufferModification::Dynamic, true);
				m_allLightsData[i].Create(sizeof(LightInfo) * c_maxLights, Render::RenderBufferModification::Dynamic, true);
			}		
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
			if (c_msaaSamples <= 1)
			{
				m_gBuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F32);	// world space position
				m_gBuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F32);	// world space normal, shininess
				m_gBuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F32);	// albedo
				m_gBuffer.AddColourAttachment(Render::FrameBuffer::RGBA_F32);	// specular colour, amount
				m_gBuffer.AddDepthStencil();									// needs to match main fb
				if (!m_gBuffer.Create())
				{
					SDE_LOG("Failed to create gbuffer");
				}
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

		const uint32_t maxTiles = m_lightTileCounts.x * m_lightTileCounts.y;
		m_lightTiles.resize(maxTiles);

		m_allInstances.Reserve(c_maxInstances);
	}

	void Renderer::Reset() 
	{ 
		m_allInstances.Reset();
		m_lights.clear();
		m_nextInstance = 0;
		m_nextDrawCall = 0;
		auto defaultDiffuse = m_textureManager->GetTexture(g_defaultTextures["DiffuseTexture"]);
		m_defaultDiffuseResidentHandle = defaultDiffuse ? defaultDiffuse->GetResidentHandle() : 0;
		auto defaultNormal = m_textureManager->GetTexture(g_defaultTextures["NormalsTexture"]);
		m_defaultNormalResidentHandle = defaultNormal ? defaultNormal->GetResidentHandle() : 0;
		auto defaultSpecular = m_textureManager->GetTexture(g_defaultTextures["SpecularTexture"]);
		m_defaultSpecularResidentHandle = defaultSpecular ? defaultSpecular->GetResidentHandle() : 0;
		if (++m_currentBuffer >= c_maxFramesAhead)
		{
			m_currentBuffer = 0;
		}
	}

	void Renderer::SetCamera(const Render::Camera& c)
	{ 
		m_camera = c;
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

	void Renderer::SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, glm::vec3 boundsMin, glm::vec3 boundsMax, const Render::Material* instanceMat)
	{
		m_allInstances.SubmitInstance(transform, mesh, shader, instanceMat, boundsMin, boundsMax);
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const Render::Mesh& mesh, const struct ShaderHandle& shader, const Render::Material* instanceMat)
	{
		SubmitInstance(transform, mesh, shader, { -FLT_MAX, -FLT_MAX, -FLT_MAX }, { FLT_MAX, FLT_MAX, FLT_MAX }, instanceMat);
	}

	void Renderer::SubmitInstances(const __m128* positions, int count, const struct ModelHandle& model, const struct ShaderHandle& shader)
	{
		m_allInstances.SubmitInstances(positions, count, model, shader);
	}

	void Renderer::SubmitInstance(const glm::mat4& transform, const struct ModelHandle& model, const struct ShaderHandle& shader, const Model::MeshPart::DrawData* partOverride, uint32_t overrideCount)
	{
		m_allInstances.SubmitInstance(transform, model, shader, partOverride, overrideCount);
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
		{
			Core::ScopedMutex lock(m_addLightMutex);
			m_lights.push_back(newLight);
		}
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
		{
			Core::ScopedMutex lock(m_addLightMutex);
			m_lights.push_back(newLight);
		}
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
		{
			Core::ScopedMutex lock(m_addLightMutex);
			m_lights.push_back(newLight);
		}
	}

	int Renderer::PopulateInstanceBuffers(RenderInstanceList& list, const EntryList& entries)
	{
		SDE_PROF_EVENT();
		
		int instanceIndex = -1;
		if (entries.size() > 0 && m_nextInstance + entries.size() < c_maxInstances)
		{
			void* bufferPtr = m_perInstanceData[m_currentBuffer].Map(Render::RenderBufferMapHint::Write, 0, entries.size() * sizeof(RenderPerInstanceData));
			RenderPerInstanceData* gpuPIDs = static_cast<RenderPerInstanceData*>(bufferPtr) + m_nextInstance;
			for (int e = 0; e < entries.size(); ++e)
			{
				const auto index = entries[e].m_dataIndex;
				gpuPIDs[e] = { list.m_transformBounds[index].m_transform, list.m_perInstanceData[index] };
			}
			m_perInstanceData[m_currentBuffer].Unmap();
			instanceIndex = m_nextInstance;
			m_nextInstance += entries.size();
		}
		return instanceIndex;
	}

	// returns the screen-space (normalized device coordinates) bounds of a projected sphere
	// center = view-space center of the sphere
	// radius = world or view space radius of the sphere
	// boxMin = bottom left of projected bounds
	// boxMax = top right of projected bounds
	void ProjectedSphereAABB(const glm::vec3& center, const float radius, float nearPlane, const glm::mat4& Projection, glm::vec3& boxMin, glm::vec3& boxMax)
	{
		if ((center.z + radius) > nearPlane) {
			return;
		}

		float d2 = glm::dot(center, center);

		float a = glm::sqrt(d2 - radius * radius);

		/// view-aligned "right" vector (right angle to the view plane from the center of the sphere. Since  "up" is always (0,n,0), replaced cross product with vec3(-c.z, 0, c.x)
		glm::vec3 right = (radius / a) * glm::vec3(-center.z, 0, center.x);
		glm::vec3 up = glm::vec3(0, radius, 0);

		glm::vec4 projectedRight = Projection * glm::vec4(right, 0);
		glm::vec4 projectedUp = Projection * glm::vec4(up, 0);

		glm::vec4 projectedCenter = Projection * glm::vec4(center, 1);

		glm::vec4 north = projectedCenter + projectedUp;
		glm::vec4 east = projectedCenter + projectedRight;
		glm::vec4 south = projectedCenter - projectedUp;
		glm::vec4 west = projectedCenter - projectedRight;
		north /= north.w;
		east /= east.w;
		west /= west.w;
		south /= south.w;

		boxMin = glm::vec3(glm::min(glm::min(glm::min(east, west), north), south));
		boxMax = glm::vec3(glm::max(glm::max(glm::max(east, west), north), south));
	}

	void Renderer::ClassifyLightTiles()
	{
		SDE_PROF_EVENT();

		const auto projectionMat = m_camera.ProjectionMatrix();
		const auto viewMat = m_camera.ViewMatrix();
		const glm::vec2 c_tileDims = glm::vec2(m_windowSize) / glm::vec2(m_lightTileCounts);

		if (m_lights.size() == 0)
		{
			for (auto& it : m_lightTiles)
			{
				it.m_currentCount = 0;
			}
		}
		else
		{
			static std::vector<glm::vec4> viewSpaceBounds;
			{
				SDE_PROF_EVENT("FindBoundsViewSpace");
				viewSpaceBounds.resize(m_lights.size());
				for (uint32_t l = 0; l < m_lights.size(); ++l)
				{
					const Light& light = m_lights[l];
					glm::vec4 bounds(0, 0, m_windowSize.x, m_windowSize.y);
					if (light.m_position.w == 1.0f)	// point light
					{
						glm::vec4 lightCenterWs = { glm::vec3(light.m_position), 1 };
						glm::vec4 centerVs = viewMat * lightCenterWs;
						glm::vec3 center = glm::vec3(centerVs) / centerVs.w;
						if (glm::length(center) >= light.m_maxDistance)	// if the light intersects the camera just shove it in all tiles
						{
							glm::vec3 bMin(-1), bMax(1);
							ProjectedSphereAABB(center, light.m_maxDistance, m_camera.NearPlane(), projectionMat, bMin, bMax);
							glm::vec2 origin = (glm::vec2(bMin.x, bMin.y) + 1.0f) * glm::vec2(m_windowSize) * 0.5f;
							glm::vec2 dims = glm::vec2(bMax.x - bMin.x, bMax.y - bMin.y) * glm::vec2(m_windowSize) * 0.5f;
							bounds = glm::vec4(origin.x, origin.y, dims.x, dims.y);
						}
					}
					viewSpaceBounds[l] = bounds;
				}
			}
			{
				SDE_PROF_EVENT("BuildTileData");
				std::atomic<int> jobsRemaining = 0;
				const int stepsPerJob = 8;
				for (uint32_t tY = 0; tY < m_lightTileCounts.y; ++tY)
				{
					for (int32_t i = 0; i < m_lightTileCounts.x; i += stepsPerJob)
					{
						const int startIndex = i;
						const int endIndex = std::min(i + stepsPerJob, m_lightTileCounts.x);
						auto runJob = [this, c_tileDims, tY, startIndex, endIndex, &jobsRemaining](void*) {
							SDE_PROF_EVENT("ClassifyLightsForTile");
							for (int32_t tX = startIndex; tX < endIndex; ++tX)
							{
								const uint32_t tileIndex = tX + (tY * m_lightTileCounts.x);
								const glm::vec2 tileOrigin = glm::vec2(tX, tY) * c_tileDims;
								uint32_t lightCount = 0;
								for (uint32_t l = 0; l < m_lights.size() && lightCount < c_maxLightsPerTile; ++l)
								{
									const Light& light = m_lights[l];
									const glm::vec2 origin = { viewSpaceBounds[l].x, viewSpaceBounds[l].y };
									const glm::vec2 dims = { viewSpaceBounds[l].z, viewSpaceBounds[l].w };
									if (origin.x + dims.x >= tileOrigin.x && origin.y + dims.y >= tileOrigin.y
										&& origin.x <= tileOrigin.x + c_tileDims.x && origin.y <= tileOrigin.y + c_tileDims.y)
									{
										m_lightTiles[tileIndex].m_lightIndices[lightCount++] = l;
									}
								}
								m_lightTiles[tileIndex].m_currentCount = lightCount;
							}
							jobsRemaining--;

						};
						jobsRemaining++;
						m_jobSystem->PushJob(runJob);
					}
				}
				while (jobsRemaining > 0)
				{
					m_jobSystem->ProcessJobThisThread();
				}
			}
		}
	
	}

	void Renderer::UploadLightTiles()
	{
		SDE_PROF_EVENT("UploadBuffer");
		m_lightTileData[m_currentBuffer].SetData(0, m_lightTiles.size() * sizeof(LightTileInfo), m_lightTiles.data());
	}

	void Renderer::DrawLightTilesDebug(RenderContext2D& r2d)
	{
		SDE_PROF_EVENT();
		const uint32_t maxTiles = m_lightTileCounts.x * m_lightTileCounts.y;
		const glm::vec4 c_lowCountColour = { 0,1,0,0.5 };
		const glm::vec4 c_highCountColour = { 1,0,0,0.5 };
		const glm::vec2 c_tileDims = glm::vec2(m_windowSize) / glm::vec2(m_lightTileCounts);
		auto defaultDiffuse = g_defaultTextures["DiffuseTexture"];
		for (uint32_t tX = 0; tX < m_lightTileCounts.x; ++tX)
		{
			for (uint32_t tY = 0; tY < m_lightTileCounts.y; ++tY)
			{
				const uint32_t tileIndex = tX + (tY * m_lightTileCounts.x);
				const uint32_t lightCount = m_lightTiles[tileIndex].m_currentCount;
				if (lightCount > 0)
				{
					float t = m_lightTiles[tileIndex].m_currentCount / (float)c_maxLightsPerTile;
					glm::vec4 c = (c_highCountColour * t) + (c_lowCountColour * (1.0f - t));
					const glm::vec2 tileOrigin = glm::vec2(tX, tY) * c_tileDims;
					r2d.DrawQuad(tileOrigin, 0, c_tileDims, { 0,0 }, { 1,1 }, c, defaultDiffuse);
				}
			}
		}
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
		globals.m_windowDims = glm::vec2(m_windowSize);
		globals.m_lightTileCounts = m_lightTileCounts;
		globals.m_viewProjMat = projectionMat * viewMat;
		uint32_t shadowMapIndex = 0;		// precalculate shadow map sampler indices
		uint32_t cubeShadowMapIndex = 0;
		static LightInfo allLights[c_maxLights];
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			allLights[l].m_colourAndAmbient = m_lights[l].m_colour;
			allLights[l].m_position = m_lights[l].m_position;
			allLights[l].m_direction = m_lights[l].m_direction;
			allLights[l].m_distanceAttenuation = { m_lights[l].m_maxDistance, m_lights[l].m_attenuationCompress };
			allLights[l].m_spotAngles = m_lights[l].m_spotlightAngles;
			bool canAddMore = m_lights[l].m_position.w == 1.0f ? cubeShadowMapIndex < c_maxShadowMaps : shadowMapIndex < c_maxShadowMaps;
			if (m_lights[l].m_shadowMap && canAddMore)
			{
				allLights[l].m_shadowParams.x = 1.0f;
				allLights[l].m_shadowParams.y = m_lights[l].m_position.w == 1.0f ? cubeShadowMapIndex++ : shadowMapIndex++;
				allLights[l].m_shadowParams.z = m_lights[l].m_shadowBias;
				allLights[l].m_lightSpaceMatrix = m_lights[l].m_lightspaceMatrix;
			}
			else
			{
				allLights[l].m_shadowParams = { 0.0f,0.0f,0.0f };
			}
		}
		const auto totalLights = static_cast<int>(std::min(m_lights.size(), c_maxLights));
		m_allLightsData[m_currentBuffer].SetData(0, sizeof(LightInfo) * totalLights, &allLights);
		globals.m_cameraPosition = glm::vec4(m_camera.Position(), 0.0);
		globals.m_hdrExposure = m_hdrExposure;
		m_globalsUniformBuffer[m_currentBuffer].SetData(0, sizeof(globals), &globals);
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

	void Renderer::PrepareDrawBuckets(const RenderInstanceList& list, const EntryList& entries, int baseIndex, std::vector<DrawBucket>& buckets)
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
				[firstInstance, &drawData, &list](const RenderInstanceList::Entry& e) -> bool {
				const auto& thisDrawData = list.m_drawData[e.m_dataIndex];
				return  thisDrawData.m_va != drawData.m_va ||
					thisDrawData.m_ib != drawData.m_ib ||
					thisDrawData.m_chunks[0].m_primitiveType != drawData.m_chunks[0].m_primitiveType ||
					thisDrawData.m_shader != drawData.m_shader ||
					thisDrawData.m_meshMaterial != drawData.m_meshMaterial;
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
			m_drawIndirectBuffer[m_currentBuffer].SetData(drawUploadIndex * sizeof(Render::DrawIndirectIndexedParams),
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
				d.SetUniforms(*b.m_shader, m_globalsUniformBuffer[m_currentBuffer], 0);
				d.BindStorageBuffer(0, m_perInstanceData[m_currentBuffer]);		// bind instancing data once per shader
				d.BindStorageBuffer(1, m_allLightsData[m_currentBuffer]);
				d.BindStorageBuffer(2, m_lightTileData[m_currentBuffer]);
				d.BindDrawIndirectBuffer(m_drawIndirectBuffer[m_currentBuffer]);
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

	void Renderer::DrawInstances(Render::Device& d, const RenderInstanceList& list, const EntryList& entries, int baseIndex, bool bindShadowmaps, Render::UniformBuffer* uniforms)
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
			auto lastMeshInstance = std::find_if(firstInstance, finalInstance, [firstInstance, &drawData, &list](const RenderInstanceList::Entry& m) -> bool {
				const auto& thisDrawData = list.m_drawData[m.m_dataIndex];
				return  thisDrawData.m_va != drawData.m_va ||
					thisDrawData.m_ib != drawData.m_ib ||
					thisDrawData.m_chunks != drawData.m_chunks ||
					thisDrawData.m_chunks[0].m_primitiveType != drawData.m_chunks[0].m_primitiveType ||
					thisDrawData.m_shader != drawData.m_shader ||
					thisDrawData.m_meshMaterial != drawData.m_meshMaterial;
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
					d.SetUniforms(*theShader, m_globalsUniformBuffer[m_currentBuffer], 0);
					d.BindStorageBuffer(0, m_perInstanceData[m_currentBuffer]);		// bind instancing data once per shader
					d.BindStorageBuffer(1, m_allLightsData[m_currentBuffer]);
					d.BindStorageBuffer(2, m_lightTileData[m_currentBuffer]);
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

	void Renderer::FindVisibleInstancesAsync(const Frustum& f, const RenderInstanceList& src, EntryList& result, OnFindVisibleComplete onComplete)
	{
		SDE_PROF_EVENT();

		auto jobs = Engine::GetSystem<JobSystem>("Jobs");
		const uint32_t partsPerJob = 5000;
		const uint32_t jobCount = src.m_count < partsPerJob ? 1 : src.m_count / partsPerJob;
		if (src.m_count == 0)
		{
			result.resize(0);
			onComplete(0);
			return;
		}
		struct JobData
		{
			std::atomic<uint32_t> m_jobsRemaining = 0;
			std::atomic<uint32_t> m_count = 0;
		};
		JobData* jobData = new JobData();
		if(result.size() < src.m_count)
		{
			SDE_PROF_EVENT("Resize results array");
			result.resize(src.m_count);
		}
		jobData->m_jobsRemaining = ceilf((float)src.m_count / partsPerJob);
		for (uint32_t p = 0; p < src.m_count; p += partsPerJob)
		{
			auto cullPartsJob = [jobData, &src, &result, p, partsPerJob, f, onComplete](void*) {
				SDE_PROF_EVENT("Cull instances");
				uint32_t firstIndex = p;
				uint32_t lastIndex = std::min((uint32_t)src.m_count, firstIndex + partsPerJob);
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
					int shuffleSortKeyCount = 0;
					int shuffleDataIndexCount = 0;
					int nonShuffleCount = 0;
					SDE_PROF_EVENT("SortResults");
					{
						std::sort(result.begin(), result.begin() + jobData->m_count,
							[&](const RenderInstanceList::Entry& s1, const RenderInstanceList::Entry& s2) {
								return SortKeyLessThan(s1.m_sortKey, s2.m_sortKey);
							});
					}
					{
						SDE_PROF_EVENT("Resize");
						result.resize(jobData->m_count);
					}
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
			int baseIndex = PopulateInstanceBuffers(m_allInstances.m_shadowCasters, instances);
			Render::UniformBuffer lightMatUniforms;
			lightMatUniforms.SetValue("ShadowLightSpaceMatrix", l.m_lightspaceMatrix);
			lightMatUniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
			d.DrawToFramebuffer(*l.m_shadowMap);
			d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
			d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
			d.SetBackfaceCulling(true, true);	// backface culling, ccw order
			d.SetBlending(false);
			d.SetScissorEnabled(false);
			d.SetWireframeDrawing(false);
			if (m_useDrawIndirect)
			{
				static std::vector<DrawBucket> drawBuckets;
				drawBuckets.reserve(2000);
				drawBuckets.resize(0);
				PrepareDrawBuckets(m_allInstances.m_shadowCasters, instances, baseIndex, drawBuckets);
				DrawBuckets(d, drawBuckets, false, &lightMatUniforms);
			}
			else
			{
				DrawInstances(d, m_allInstances.m_shadowCasters, instances, baseIndex, false, &lightMatUniforms);
			}
			m_frameStats.m_totalShadowInstances += m_allInstances.m_shadowCasters.m_count;
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
				int baseIndex = PopulateInstanceBuffers(m_allInstances.m_shadowCasters, instances);
				d.DrawToFramebuffer(*l.m_shadowMap, cubeFace);
				d.ClearFramebufferDepth(*l.m_shadowMap, FLT_MAX);
				d.SetViewport(glm::ivec2(0, 0), l.m_shadowMap->Dimensions());
				d.SetBackfaceCulling(true, true);	// backface culling, ccw order
				d.SetBlending(false);
				d.SetScissorEnabled(false);
				d.SetWireframeDrawing(false);
				uniforms.SetValue("ShadowLightSpaceMatrix", shadowTransforms[cubeFace]);
				uniforms.SetValue("ShadowLightIndex", (int32_t)(&l - &m_lights[0]));
				if (m_useDrawIndirect)
				{
					static std::vector<DrawBucket> drawBuckets;
					drawBuckets.reserve(2000);
					drawBuckets.resize(0);
					PrepareDrawBuckets(m_allInstances.m_shadowCasters, instances, baseIndex, drawBuckets);
					DrawBuckets(d, drawBuckets, false, &uniforms);
				}
				else
				{
					DrawInstances(d, m_allInstances.m_shadowCasters, instances, baseIndex, false, &uniforms);
				}
				m_frameStats.m_totalShadowInstances += m_allInstances.m_shadowCasters.m_count;
				m_frameStats.m_renderedShadowInstances += instances.size();
			}
			return instanceListStartIndex + 6;
		}
	}

	void Renderer::ForEachUsedRT(std::function<void(const char*, Render::FrameBuffer&)> rtFn)
	{
		rtFn("MainColourDepth", m_mainFramebuffer);
		if (m_mainFramebuffer.GetMSAASamples() > 1)
		{
			rtFn("MainResolved", m_mainFramebufferResolved);
		}
		else if(m_drawDeferred)
		{
			rtFn("GBuffer", m_gBuffer);
		}
		rtFn("BloomBrightness", m_bloomBrightnessBuffer);
		rtFn("BloomBlur0", *m_bloomBlurBuffers[0]);
		rtFn("BloomBlur1", *m_bloomBlurBuffers[1]);
	}

	void Renderer::BeginCullingAsync()
	{
		SDE_PROF_EVENT();
		m_opaquesFwdReady = false;
		m_opaquesDeferredReady = false;
		m_transparentsReady = false;
		m_shadowsInFlight = 0;

		// Kick off the shadow caster culling first
		int shadowCasterListId = 0;	// tracks the current result list to write to
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr && m_lights[l].m_updateShadowmap)
			{
				while (m_visibleShadowCasters.size() < shadowCasterListId + 1)
				{
					auto newInstanceList = std::make_unique<EntryList>();
					newInstanceList->reserve(m_allInstances.m_shadowCasters.m_count);
					m_visibleShadowCasters.emplace_back(std::move(newInstanceList));
				}
				if (m_lights[l].m_position.w == 0.0f || m_lights[l].m_position.w == 2.0f)		// directional / spot
				{
					++m_shadowsInFlight;
					Frustum shadowFrustum(m_lights[l].m_lightspaceMatrix);
					FindVisibleInstancesAsync(shadowFrustum, m_allInstances.m_shadowCasters,
						*m_visibleShadowCasters[shadowCasterListId], [shadowCasterListId, this](size_t count) {
							--m_shadowsInFlight;
						});
					shadowCasterListId++;
				}
				else
				{
					while (m_visibleShadowCasters.size() < shadowCasterListId + 6)
					{
						auto newInstanceList = std::make_unique<EntryList>();
						newInstanceList->reserve(m_allInstances.m_shadowCasters.m_count);
						m_visibleShadowCasters.emplace_back(std::move(newInstanceList));
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
						++m_shadowsInFlight;
						FindVisibleInstancesAsync(shadowFrustum, m_allInstances.m_shadowCasters,
							*m_visibleShadowCasters[shadowCasterListId], [shadowCasterListId, this](size_t count) {
								--m_shadowsInFlight;
							});
						shadowCasterListId++;
					}
				}
			}
		}

		m_visibleOpaquesDeferred.reserve(m_allInstances.m_opaquesDeferred.m_count);
		m_visibleOpaquesFwd.reserve(m_allInstances.m_opaquesForward.m_count);
		m_visibleTransparents.reserve(m_allInstances.m_transparents.m_count);
		Frustum viewFrustum(m_camera.ProjectionMatrix() * m_camera.ViewMatrix());
		FindVisibleInstancesAsync(viewFrustum, m_allInstances.m_opaquesForward, m_visibleOpaquesFwd, [&](size_t resultCount) {
			m_opaquesFwdReady = true;
		});
		FindVisibleInstancesAsync(viewFrustum, m_allInstances.m_opaquesDeferred, m_visibleOpaquesDeferred, [&](size_t r) {
			m_opaquesDeferredReady = true;
		});
		FindVisibleInstancesAsync(viewFrustum, m_allInstances.m_transparents, m_visibleTransparents, [&](size_t resultCount) {
			m_transparentsReady = true;
		});
	}

	void Renderer::RenderShadowMaps(Render::Device& d)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("Wait for shadow lists");
			while (m_shadowsInFlight > 0)
			{
				m_jobSystem->ProcessJobThisThread();
			}
		}
		int startIndex = 0;
		for (int l = 0; l < m_lights.size() && l < c_maxLights; ++l)
		{
			if (m_lights[l].m_shadowMap != nullptr && m_lights[l].m_updateShadowmap)
			{
				startIndex = RenderShadowmap(d, m_lights[l], m_visibleShadowCasters, startIndex);
			}
		}
	}

	void Renderer::RenderOpaquesDeferred(Render::Device& d)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("Wait for opaques");
			while (!m_opaquesDeferredReady)
			{
				m_jobSystem->ProcessJobThisThread();
			}
		}
		// gbuffer
		d.SetWireframeDrawing(m_showWireframe);
		d.DrawToFramebuffer(m_gBuffer);
		d.SetViewport(glm::ivec2(0, 0), m_mainFramebuffer.Dimensions());
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(false);				// no blending for opaques
		d.SetScissorEnabled(false);			// (don't) scissor me timbers
		int baseInstance = PopulateInstanceBuffers(m_allInstances.m_opaquesDeferred, m_visibleOpaquesDeferred);
		if (m_useDrawIndirect)
		{
			static std::vector<DrawBucket> drawBuckets;
			drawBuckets.reserve(2000);
			drawBuckets.resize(0);
			PrepareDrawBuckets(m_allInstances.m_opaquesDeferred, m_visibleOpaquesDeferred, baseInstance, drawBuckets);
			DrawBuckets(d, drawBuckets, true, nullptr);
		}
		else
		{
			DrawInstances(d, m_allInstances.m_opaquesDeferred, m_visibleOpaquesDeferred, baseInstance, true, nullptr);
		}

		// lighting pass -> main fb
		auto lightShader = m_shaderManager->GetShader(m_deferredLightingShader);
		if (lightShader != nullptr)
		{
			d.SetWireframeDrawing(false);
			d.DrawToFramebuffer(m_mainFramebuffer);
			d.SetViewport(glm::ivec2(0, 0), m_mainFramebuffer.Dimensions());
			d.SetBackfaceCulling(false, true);	// backface culling off, ccw order
			d.BindShaderProgram(*lightShader);		// must be set before uniforms
			d.SetDepthState(false, false);			// dont read/write depth here, we blit it later via resolve
			d.BindUniformBufferIndex(*lightShader, "Globals", 0);
			d.SetUniforms(*lightShader, m_globalsUniformBuffer[m_currentBuffer], 0);
			d.BindStorageBuffer(1, m_allLightsData[m_currentBuffer]);
			d.BindStorageBuffer(2, m_lightTileData[m_currentBuffer]);
			uint32_t gbufferPosHandle = lightShader->GetUniformHandle("GBuffer_Pos");
			uint32_t gbufferNormHandle = lightShader->GetUniformHandle("GBuffer_NormalShininess");
			uint32_t gbufferAlbedoHandle = lightShader->GetUniformHandle("GBuffer_Albedo");
			uint32_t gbufferSpecHandle = lightShader->GetUniformHandle("GBuffer_Specular");
			if(gbufferPosHandle != -1)
				d.SetSampler(gbufferPosHandle, m_gBuffer.GetColourAttachment(0).GetResidentHandle());
			if (gbufferNormHandle != -1)
				d.SetSampler(gbufferNormHandle, m_gBuffer.GetColourAttachment(1).GetResidentHandle());
			if (gbufferAlbedoHandle != -1)
				d.SetSampler(gbufferAlbedoHandle, m_gBuffer.GetColourAttachment(2).GetResidentHandle());
			if (gbufferSpecHandle != -1)
				d.SetSampler(gbufferSpecHandle, m_gBuffer.GetColourAttachment(3).GetResidentHandle());
			BindShadowmaps(d, *lightShader, 0);
			m_targetBlitter.RunOnTarget(d, m_mainFramebuffer, *lightShader);

			// blit depth from gbuffer to main buffer
			m_gBuffer.Resolve(m_mainFramebuffer, Render::FrameBuffer::ResolveType::Depth);
		}
	}

	void Renderer::RenderOpaquesForward(Render::Device& d)
	{
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("Wait for opaques");
			while (!m_opaquesFwdReady)
			{
				m_jobSystem->ProcessJobThisThread();
			}
		}

		d.DrawToFramebuffer(m_mainFramebuffer);
		d.SetViewport(glm::ivec2(0, 0), m_mainFramebuffer.Dimensions());
		d.SetBackfaceCulling(true, true);	// backface culling, ccw order
		d.SetBlending(false);				// no blending for opaques
		d.SetScissorEnabled(false);			// (don't) scissor me timbers
		d.SetWireframeDrawing(m_showWireframe);
		int baseInstance = PopulateInstanceBuffers(m_allInstances.m_opaquesForward, m_visibleOpaquesFwd);
		if (m_useDrawIndirect)
		{
			static std::vector<DrawBucket> drawBuckets;
			drawBuckets.reserve(2000);
			drawBuckets.resize(0);
			PrepareDrawBuckets(m_allInstances.m_opaquesForward, m_visibleOpaquesFwd, baseInstance, drawBuckets);
			DrawBuckets(d, drawBuckets, true, nullptr);
		}
		else
		{
			DrawInstances(d, m_allInstances.m_opaquesForward, m_visibleOpaquesFwd, baseInstance, true, nullptr);
		}
	}

	void Renderer::RenderTransparents(Render::Device& d)
	{
		d.SetDepthState(true, false);		// enable z-test, disable write
		d.SetBlending(true);
		d.SetWireframeDrawing(m_showWireframe);
		{
			SDE_PROF_EVENT("Wait for transparents");
			while (!m_transparentsReady)
			{
				m_jobSystem->ProcessJobThisThread();
			}
		}
		m_frameStats.m_totalTransparentInstances = m_allInstances.m_transparents.m_count;
		int baseInstance = PopulateInstanceBuffers(m_allInstances.m_transparents, m_visibleTransparents);
		if (m_useDrawIndirect)
		{
			static std::vector<DrawBucket> drawBuckets;
			drawBuckets.reserve(2000);
			drawBuckets.resize(0);
			PrepareDrawBuckets(m_allInstances.m_transparents, m_visibleTransparents, baseInstance, drawBuckets);
			DrawBuckets(d, drawBuckets, true, nullptr);
		}
		else
		{
			DrawInstances(d, m_allInstances.m_transparents, m_visibleTransparents, baseInstance, true, nullptr);
		}
	}

	void Renderer::RenderPostFx(Render::Device& d, Render::FrameBuffer& src)
	{
		// bloom brightness pass
		d.SetDepthState(false, false);
		d.SetBlending(false);
		d.SetWireframeDrawing(false);
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
			m_targetBlitter.TargetToTarget(d, src, m_bloomBrightnessBuffer, *brightnessShader);
		}

		// bloom gaussian blur ping-pong
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
				d.SetSampler(sampler, src.GetColourAttachment(0).GetResidentHandle());
			}
			m_targetBlitter.TargetToTarget(d, *m_bloomBlurBuffers[0], m_bloomBrightnessBuffer, *combineShader);
		}

		// blit to main buffer + tonemap
		d.SetBlending(true);
		auto tonemapShader = m_shaderManager->GetShader(m_tonemapShader);
		if (tonemapShader)
		{
			m_targetBlitter.TargetToTarget(d, m_bloomBrightnessBuffer, src, *tonemapShader);
		}
	}

	void Renderer::RenderAll(Render::Device& d)
	{
		SDE_PROF_EVENT();
		m_frameStats = {};
		m_frameStats.m_instancesSubmitted = m_allInstances.m_opaquesDeferred.m_count + m_allInstances.m_opaquesForward.m_count + m_allInstances.m_transparents.m_count;
		m_frameStats.m_activeLights = std::min(m_lights.size(), c_maxLights);

		CullLights();
		ClassifyLightTiles();
		BeginCullingAsync();

		{
			SDE_PROF_EVENT("Clear framebuffers");
			d.SetDepthState(true, true);	// make sure depth write is enabled before clearing!
			d.ClearFramebufferColourDepth(m_mainFramebuffer, m_clearColour, FLT_MAX);
			d.ClearFramebufferColourDepth(m_gBuffer, { 0,0,0,0 }, FLT_MAX);
		}

		// upload global constants while waiting for culling + clears
		const auto projectionMat = m_camera.ProjectionMatrix();
		const auto viewMat = m_camera.ViewMatrix();
		UpdateGlobals(projectionMat, viewMat);
		UploadLightTiles();

		// shadow maps first to be drawn
		RenderShadowMaps(d);

		// main geometry passes
		if (m_drawDeferred && c_msaaSamples <= 1)
		{
			RenderOpaquesDeferred(d);
		}
		RenderOpaquesForward(d);
		m_frameStats.m_renderedOpaqueInstances = m_visibleOpaquesDeferred.size() + m_visibleOpaquesFwd.size();
		m_frameStats.m_totalOpaqueInstances = m_allInstances.m_opaquesDeferred.m_count + m_allInstances.m_opaquesForward.m_count;

		RenderTransparents(d);
		m_frameStats.m_renderedTransparentInstances = m_visibleTransparents.size();

		Render::FrameBuffer* mainFb = &m_mainFramebuffer;
		if(m_mainFramebuffer.GetMSAASamples() > 1)
		{
			SDE_PROF_EVENT("ResolveMSAA");
			m_mainFramebuffer.Resolve(m_mainFramebufferResolved);
			mainFb = &m_mainFramebufferResolved;
		}
		RenderPostFx(d, *mainFb);

		// blit main buffer to backbuffer
		d.SetDepthState(false, false);
		d.SetBlending(true);
		auto blitShader = m_shaderManager->GetShader(m_blitShader);
		if (blitShader)
		{
			m_targetBlitter.TargetToBackbuffer(d, *mainFb, *blitShader, m_windowSize);
		}
	}
}