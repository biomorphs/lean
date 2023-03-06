#include "render_instance_list.h"
#include "model_manager.h"
#include "texture_manager.h"
#include "shader_manager.h"
#include "system_manager.h"
#include "graphics_system.h"
#include "camera_system.h"
#include "renderer.h"

namespace Engine
{
	uint32_t RenderInstanceList::AddInstances(int instanceCount)
	{
		uint32_t oldCount = m_count.fetch_add(instanceCount);
		if (oldCount + instanceCount < m_maxInstances)
		{
			return oldCount;
		}
		else
		{
			return -1;
		}
	}

	void RenderInstanceList::Reserve(size_t count)
	{
		m_transformBounds.resize(count);
		m_drawData.resize(count);
		m_perInstanceData.resize(count);
		m_entries.resize(count);
		m_count = glm::min((uint32_t)count, m_count.load());	// clamp count to new max size
		m_maxInstances = count;
	}

	void RenderInstanceList::Reset()
	{
		m_count = 0;
	}

	void RenderInstanceList::SetInstance(uint32_t index, __m128i sortKey, const glm::mat4& trns, const Render::VertexArray* va, const Render::RenderBuffer* ib, const Render::MeshChunk* chunks,
		uint32_t chunkCount, const Render::Material* meshMaterial, Render::ShaderProgram* shader, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
		const PerInstanceData& pid)
	{
		assert(index < m_entries.size() && index >= 0);
		m_entries[index] = { sortKey, (uint32_t)index };
		m_transformBounds[index] = { trns, aabbMin, aabbMax };
		m_drawData[index] = { shader, va, ib, meshMaterial, chunks, chunkCount };
		m_perInstanceData[index] = pid;
	}

	inline __m128i ShadowCasterKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::MeshChunk* chunks, const Render::Material* meshMat)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));

		const void* chunkPtrVoid = static_cast<const void*>(chunks);
		uintptr_t chunkPtr = reinterpret_cast<uintptr_t>(chunkPtrVoid);
		uint32_t chunkPtrLow = static_cast<uint32_t>((chunkPtr & 0x00000000ffffffff));

		const void* meshMatPtrVoid = static_cast<const void*>(meshMat);
		uintptr_t meshMatPtr = reinterpret_cast<uintptr_t>(meshMatPtrVoid);
		uint32_t meshMatPtrLow = static_cast<uint32_t>((meshMatPtr & 0x00000000ffffffff));

		__m128i result = _mm_set_epi32(shader.m_index, vaPtrLow, chunkPtrLow, meshMatPtrLow);

		return result;
	}

	inline __m128i OpaqueKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::MeshChunk* chunks, const Render::Material* meshMat)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));

		const void* chunkPtrVoid = static_cast<const void*>(chunks);
		uintptr_t chunkPtr = reinterpret_cast<uintptr_t>(chunkPtrVoid);
		uint32_t chunkPtrLow = static_cast<uint32_t>((chunkPtr & 0x00000000ffffffff));

		const void* meshMatPtrVoid = static_cast<const void*>(meshMat);
		uintptr_t meshMatPtr = reinterpret_cast<uintptr_t>(meshMatPtrVoid);
		uint32_t meshMatPtrLow = static_cast<uint32_t>((meshMatPtr & 0x00000000ffffffff));

		__m128i result = _mm_set_epi32(shader.m_index, vaPtrLow, chunkPtrLow, meshMatPtrLow);

		return result;
	}

	inline __m128i TransparentKey(const ShaderHandle& shader, const Render::VertexArray* va, const Render::MeshChunk* chunks, float distanceToCamera)
	{
		const void* vaPtrVoid = static_cast<const void*>(va);
		uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
		uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));

		const void* chunkPtrVoid = static_cast<const void*>(chunks);
		uintptr_t chunkPtr = reinterpret_cast<uintptr_t>(chunkPtrVoid);
		uint32_t chunkPtrLow = static_cast<uint32_t>((chunkPtr & 0x00000000ffffffff));
		uint32_t vaChunkCombined = vaPtrLow ^ chunkPtrLow;

		uint32_t distanceToCameraAsInt = static_cast<uint32_t>(distanceToCamera * 1000.0f);
		__m128i result = _mm_set_epi32(distanceToCameraAsInt, shader.m_index, vaChunkCombined, 0);

		return result;
	}

	void RenderInstances::SubmitInstance(const glm::mat4& trns, const Render::Mesh& mesh, const ShaderHandle& shader, const Render::Material* matOverride, glm::vec3 boundsMin, glm::vec3 boundsMax)
	{
		static auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		static auto textures = Engine::GetSystem<TextureManager>("Textures");
		static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		const auto& mainCam = Engine::GetSystem<Engine::CameraSystem>("Cameras")->MainCamera();	// careful if async
		const auto theShader = shaders->GetShader(shader);
		const auto& renderer = graphics->Renderer();
		if (theShader != nullptr)
		{
			ShaderHandle shadowShader = shaders->GetShadowsShader(shader);
			Render::ShaderProgram* shadowShaderPtr = shaders->GetShader(shadowShader);
			const Render::VertexArray* va = &mesh.GetVertexArray();
			const Render::RenderBuffer* ib = mesh.GetIndexBuffer().get();
			const Render::Material* mat = matOverride != nullptr ? matOverride : &mesh.GetMaterial();

			PerInstanceData pid;
			pid.m_diffuseTexture = renderer.GetDefaultDiffuseTexture();
			pid.m_normalsTexture = renderer.GetDefaultNormalsTexture();
			pid.m_specularTexture = renderer.GetDefaultSpecularTexture();
			pid.m_diffuseOpacity = { 1,1,1,1 };
			pid.m_specular = { 0,0,0,0 };
			pid.m_shininess = { 0,0,0,0 };
			if (mat->GetCastsShadows() && shadowShaderPtr != nullptr)
			{
				int baseIndex = m_shadowCasters.AddInstances(1);
				const auto shadowSortKey = ShadowCasterKey(shadowShader, va, mesh.GetChunks().data(), nullptr);
				if (baseIndex != -1)
				{
					m_shadowCasters.SetInstance(baseIndex, shadowSortKey, trns, va, ib, mesh.GetChunks().data(), mesh.GetChunks().size(),
						nullptr, shadowShaderPtr, boundsMin, boundsMax, pid);
				}
			}
			if (mat->GetIsTransparent())
			{
				int baseIndex = m_transparents.AddInstances(1);
				if (baseIndex != -1)
				{
					const float distanceToCamera = glm::length(glm::vec3(trns[3]) - mainCam.Position());
					auto sortKey = TransparentKey(shader, va, mesh.GetChunks().data(), distanceToCamera);
					m_transparents.SetInstance(baseIndex, sortKey, trns, va, ib, mesh.GetChunks().data(), mesh.GetChunks().size(),
						nullptr, theShader, boundsMin, boundsMax, pid);
				}
			}
			else
			{
				int baseIndex = m_opaques.AddInstances(1);
				const auto opaqueSortKey = OpaqueKey(shader, va, mesh.GetChunks().data(), nullptr);
				if (baseIndex != -1)
				{
					m_opaques.SetInstance(baseIndex, opaqueSortKey, trns, va, ib, mesh.GetChunks().data(), mesh.GetChunks().size(),
						nullptr, theShader, boundsMin, boundsMax, pid);
				}
			}
		}
	}

	void RenderInstances::SubmitInstance(const glm::mat4& trns, const ModelHandle& model, const ShaderHandle& shader, const Model::MeshPart::DrawData* partOverride, uint32_t overrideCount)
	{
		static auto models = Engine::GetSystem<Engine::ModelManager>("Models");
		static auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		static auto textures = Engine::GetSystem<TextureManager>("Textures");
		static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		const auto& mainCam = Engine::GetSystem<Engine::CameraSystem>("Cameras")->MainCamera();	// careful if async
		const auto theModel = models->GetModel(model);
		const auto theShader = shaders->GetShader(shader);
		const auto& renderer = graphics->Renderer();
		if (theModel != nullptr && theShader != nullptr)
		{
			ShaderHandle shadowShader = shaders->GetShadowsShader(shader);
			Render::ShaderProgram* shadowShaderPtr = shaders->GetShader(shadowShader);
			const Render::VertexArray* va = models->GetVertexArray();
			const Render::RenderBuffer* ib = models->GetIndexBuffer();
			const int partCount = theModel->MeshParts().size();
			PerInstanceData pid;
			for (int partIndex = 0; partIndex < partCount; ++partIndex)
			{
				const auto& meshPart = theModel->MeshParts()[partIndex];
				const Model::MeshPart::DrawData& partDrawData = overrideCount > partIndex ? partOverride[partIndex] : meshPart.m_drawData;
				const auto diffuse = textures->GetTexture(partDrawData.m_diffuseTexture);
				const auto normal = textures->GetTexture(partDrawData.m_normalsTexture);
				const auto specular = textures->GetTexture(partDrawData.m_specularTexture);
				const glm::vec3 boundsMin = meshPart.m_boundsMin, boundsMax = meshPart.m_boundsMax;
				pid.m_diffuseOpacity = partDrawData.m_diffuseOpacity;
				pid.m_specular = partDrawData.m_specular;
				pid.m_shininess = partDrawData.m_shininess;
				pid.m_diffuseTexture = diffuse ? diffuse->GetResidentHandle() : renderer.GetDefaultDiffuseTexture();
				pid.m_normalsTexture = normal ? normal->GetResidentHandle() : renderer.GetDefaultNormalsTexture();
				pid.m_specularTexture = specular ? specular->GetResidentHandle() : renderer.GetDefaultSpecularTexture();
				const glm::mat4 partTrns = trns * meshPart.m_transform;
				if (partDrawData.m_castsShadows && shadowShaderPtr != nullptr)
				{
					int baseIndex = m_shadowCasters.AddInstances(1);
					const auto shadowSortKey = ShadowCasterKey(shadowShader, va, meshPart.m_chunks.data(), nullptr);
					if(baseIndex != -1)
					{
						m_shadowCasters.SetInstance(baseIndex, shadowSortKey, partTrns, va, ib, meshPart.m_chunks.data(), meshPart.m_chunks.size(),
							nullptr, shadowShaderPtr, boundsMin, boundsMax, pid);
					}
				}
				if (partDrawData.m_isTransparent || pid.m_diffuseOpacity.a != 1.0f)
				{
					int baseIndex = m_transparents.AddInstances(1);
					if(baseIndex != -1)
					{
						const float distanceToCamera = glm::length(glm::vec3(partTrns[3]) - mainCam.Position());
						auto sortKey = TransparentKey(shader, va, meshPart.m_chunks.data(), distanceToCamera);
						m_transparents.SetInstance(baseIndex, sortKey, partTrns, va, ib, meshPart.m_chunks.data(), meshPart.m_chunks.size(),
							nullptr, theShader, boundsMin, boundsMax, pid);
					}
				}
				else
				{
					int baseIndex = m_opaques.AddInstances(1);
					const auto opaqueSortKey = OpaqueKey(shader, va, meshPart.m_chunks.data(), nullptr);
					if(baseIndex != -1)
					{
						m_opaques.SetInstance(baseIndex, opaqueSortKey, partTrns, va, ib, meshPart.m_chunks.data(), meshPart.m_chunks.size(),
							nullptr, theShader, boundsMin, boundsMax, pid);
					}
				}
			}
		}
	}

	void RenderInstances::SubmitInstances(const __m128* positions, int count, const ModelHandle& model, const ShaderHandle& shader)
	{
		SDE_PROF_EVENT();
		static auto models = Engine::GetSystem<Engine::ModelManager>("Models");
		static auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
		static auto textures = Engine::GetSystem<TextureManager>("Textures");
		static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		const auto& mainCam = Engine::GetSystem<Engine::CameraSystem>("Cameras")->MainCamera();	// careful if async
		const auto theModel = models->GetModel(model);
		const auto theShader = shaders->GetShader(shader);
		const auto& renderer = graphics->Renderer();
		if (theModel != nullptr && theShader != nullptr && count > 0)
		{
			ShaderHandle shadowShader = shaders->GetShadowsShader(shader);
			Render::ShaderProgram* shadowShaderPtr = shaders->GetShader(shadowShader);
			const Render::VertexArray* va = models->GetVertexArray();
			const Render::RenderBuffer* ib = models->GetIndexBuffer();
			const int partCount = theModel->MeshParts().size();
			for (int partIndex = 0; partIndex < partCount; ++partIndex)
			{
				const auto& meshPart = theModel->MeshParts()[partIndex];
				const Model::MeshPart::DrawData& partDrawData = meshPart.m_drawData;
				const auto diffuse = textures->GetTexture(partDrawData.m_diffuseTexture);
				const auto normal = textures->GetTexture(partDrawData.m_normalsTexture);
				const auto specular = textures->GetTexture(partDrawData.m_specularTexture);
				const glm::vec3 boundsMin = meshPart.m_boundsMin, boundsMax = meshPart.m_boundsMax;

				PerInstanceData pid;
				pid.m_diffuseOpacity = partDrawData.m_diffuseOpacity;
				pid.m_specular = partDrawData.m_specular;
				pid.m_shininess = partDrawData.m_shininess;
				pid.m_diffuseTexture = diffuse ? diffuse->GetResidentHandle() : renderer.GetDefaultDiffuseTexture();
				pid.m_normalsTexture = normal ? normal->GetResidentHandle() : renderer.GetDefaultNormalsTexture();
				pid.m_specularTexture = specular ? specular->GetResidentHandle() : renderer.GetDefaultSpecularTexture();;
				__declspec(align(16)) glm::vec4 instancePos;

				if (partDrawData.m_castsShadows && shadowShaderPtr != nullptr)
				{
					int baseIndex = m_shadowCasters.AddInstances(count);
					const auto shadowSortKey = ShadowCasterKey(shadowShader, va, meshPart.m_chunks.data(), nullptr);
					for (int i = 0; i < count && baseIndex != -1; ++i)
					{
						_mm_store_ps(glm::value_ptr(instancePos), positions[i]);
						const glm::mat4 instanceTransform = glm::translate(glm::vec3(instancePos)) * meshPart.m_transform;
						m_shadowCasters.SetInstance(baseIndex + i, shadowSortKey, instanceTransform, va, ib, meshPart.m_chunks.data(), meshPart.m_chunks.size(),
							nullptr, shadowShaderPtr, boundsMin, boundsMax, pid);
					}
				}
				if (partDrawData.m_isTransparent || pid.m_diffuseOpacity.a != 1.0f)
				{
					int baseIndex = m_transparents.AddInstances(count);
					for (int i = 0; i < count && baseIndex != -1; ++i)
					{
						_mm_store_ps(glm::value_ptr(instancePos), positions[i]);
						const glm::mat4 instanceTransform = glm::translate(glm::vec3(instancePos)) * meshPart.m_transform;
						const float distanceToCamera = glm::length(glm::vec3(instanceTransform[3]) - mainCam.Position());
						auto sortKey = TransparentKey(shader, va, meshPart.m_chunks.data(), distanceToCamera);
						m_transparents.SetInstance(baseIndex + i, sortKey, instanceTransform, va, ib, meshPart.m_chunks.data(), meshPart.m_chunks.size(),
							nullptr, theShader, boundsMin, boundsMax, pid);
					}
				}
				else
				{
					int baseIndex = m_opaques.AddInstances(count);
					const auto opaqueSortKey = OpaqueKey(shader, va, meshPart.m_chunks.data(), nullptr);
					for (int i = 0; i < count && baseIndex != -1; ++i)
					{
						_mm_store_ps(glm::value_ptr(instancePos), positions[i]);
						const glm::mat4 instanceTransform = glm::translate(glm::vec3(instancePos)) * meshPart.m_transform;
						m_opaques.SetInstance(baseIndex + i, opaqueSortKey, instanceTransform, va, ib, meshPart.m_chunks.data(), meshPart.m_chunks.size(),
							nullptr, theShader, boundsMin, boundsMax, pid);
					}
				}
			}
		}
	}

	void RenderInstances::Reset()
	{
		m_opaques.Reset();
		m_transparents.Reset();
		m_shadowCasters.Reset();
	}

	void RenderInstances::Reserve(size_t count)
	{
		m_opaques.Reserve(count);
		m_transparents.Reserve(count);
		m_shadowCasters.Reserve(count);
	}
}