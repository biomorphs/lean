#include "raycast_system.h"
#include "debug_gui_system.h"
#include "job_system.h"
#include "debug_render.h"
#include "render_system.h"
#include "components/component_transform.h"
#include "sdf_mesh_octree.h"
#include "components/component_sdf_mesh.h"
#include "system_manager.h"
#include "entity/entity_system.h"
#include "entity/entity_handle.h"
#include "graphics_system.h"
#include "physics_system.h"
#include "render/mesh.h"
#include "render/render_buffer.h"
#include "render/device.h"
#include "texture_manager.h"
#include "components/component_material.h"
#include "material_helpers.h"
#include "render/material.h"
#include "core/log.h"
#include "core/profiler.h"

namespace Engine
{
	const uint32_t c_maxActiveRays = 1024 * 128;
	const uint32_t c_maxActiveIndices = c_maxActiveRays * 4;

	struct RaycastShaderOutput
	{
		glm::vec4 m_normalTPos;
		uint32_t m_entityID;
		uint32_t m_padding[3];
	};

	bool RaycastSystem::ProcessResults::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		struct closestHit
		{
			glm::vec4 m_normalTPoint = glm::vec4(FLT_MAX);
			EntityHandle m_hitEntity;
		};
		std::vector<closestHit> closestResults(m_parent->m_activeRays.size());	// keep track of the hit closest to start for each ray

		// do the physics raycasts now to give gpu time to catch up
		{
			SDE_PROF_EVENT("PhysicsRaycasts");
			for (int r = 0; r < m_parent->m_activeRays.size(); ++r)
			{
				const auto& ray = m_parent->m_activeRays[r];
				glm::vec3 physHitNormal;
				float tHitPoint;
				auto physHitEntity = m_parent->m_physics->Raycast(ray.m_p0, ray.m_p1, tHitPoint, physHitNormal);
				if (physHitEntity.IsValid())
				{
					closestResults[r].m_normalTPoint = glm::vec4(physHitNormal, tHitPoint);
					closestResults[r].m_hitEntity = physHitEntity;
				}
			}
		}

		// process SDF raycasts
		if (m_parent->m_activeRays.size() > 0)
		{
			SDE_PROF_EVENT("CollectSDFResults");
			m_parent->m_renderSys->GetDevice()->MemoryBarrier(Render::BarrierType::BufferData);
			auto r = m_parent->m_renderSys->GetDevice()->WaitOnFence(m_parent->m_activeRayFence, 0);
			if (r == Render::FenceResult::Signalled)
			{
				// find the closest hit for each ray
				const void* outputBuffer = m_parent->m_raycastOutputBuffer->Map(Render::RenderBufferMapHint::Read, 0, m_parent->m_raycastOutputBuffer->GetSize());
				auto result = reinterpret_cast<const RaycastShaderOutput*>(outputBuffer);
				for (auto index : m_parent->m_activeRayIndices)
				{
					if (result->m_normalTPos.w != -1.0f && result->m_normalTPos.w < closestResults[index].m_normalTPoint.w)
					{
						closestResults[index].m_normalTPoint = result->m_normalTPos;
						closestResults[index].m_hitEntity = result->m_entityID;
					}
					++result;
				}
				m_parent->m_raycastOutputBuffer->Unmap();
			}
		}

		// finally, run callbacks
		{
			SDE_PROF_EVENT("FireCallbacks");
			for (int r = 0; r < m_parent->m_activeRays.size(); ++r)
			{
				const auto& ray = m_parent->m_activeRays[r];
				if (closestResults[r].m_normalTPoint.w != FLT_MAX)
				{
					const auto hitPoint = ray.m_p0 + (ray.m_p1 - ray.m_p0) * closestResults[r].m_normalTPoint.w;
					m_parent->m_activeRays[r].m_onHit(ray.m_p0, ray.m_p1, hitPoint, glm::vec3(closestResults[r].m_normalTPoint), closestResults[r].m_hitEntity);
				}
				else
				{
					m_parent->m_activeRays[r].m_onMiss(ray.m_p0, ray.m_p1);
				}
			}
		}

		m_parent->m_activeRays.clear();
		m_parent->m_activeRayIndices.clear();

		return true;
	}

	RaycastSystem::RaycastSystem()
	{
	}

	RaycastSystem::~RaycastSystem()
	{
	}

	void RaycastSystem::RaycastAsync(glm::vec3 p0, glm::vec3 p1, RayHitFn onHit, RayMissFn onMiss)
	{
		m_pendingRays.push_back({ p0,p1,onHit,onMiss });
	}

	RaycastSystem::ProcessResults* RaycastSystem::MakeResultProcessor()
	{
		return new ProcessResults(this);
	}

	bool RaycastSystem::PreInit(SystemManager& manager)
	{
		SDE_PROF_EVENT();

		m_jobSystem = (Engine::JobSystem*)manager.GetSystem("Jobs");
		m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");
		m_debugGuiSystem = (DebugGuiSystem*)manager.GetSystem("DebugGui");
		m_scriptSystem = (Engine::ScriptSystem*)manager.GetSystem("Script");
		m_renderSys = (Engine::RenderSystem*)manager.GetSystem("Render");
		m_graphics = (GraphicsSystem*)manager.GetSystem("Graphics");
		m_physics = (PhysicsSystem*)manager.GetSystem("Physics");

		return true;
	}

	bool RaycastSystem::Initialise()
	{
		SDE_PROF_EVENT();

		m_raycastOutputBuffer = std::make_unique<Render::RenderBuffer>();
		m_raycastOutputBuffer->Create(c_maxActiveIndices * sizeof(RaycastShaderOutput), Render::RenderBufferModification::Dynamic, true, true);

		m_activeRayBuffer = std::make_unique<Render::RenderBuffer>();
		m_activeRayBuffer->Create(c_maxActiveRays * sizeof(float) * 6, Render::RenderBufferModification::Dynamic, true);

		m_activeRayIndexBuffer = std::make_unique<Render::RenderBuffer>();
		m_activeRayIndexBuffer->Create(c_maxActiveIndices * sizeof(uint32_t), Render::RenderBufferModification::Dynamic, true, true);

		//// expose script functions
		auto raycasts = m_scriptSystem->Globals()["Raycast"].get_or_create<sol::table>();
		raycasts["DoAsync"] = [this](glm::vec3 start, glm::vec3 end, RayHitFn hit, RayMissFn miss) {
			RaycastAsync(start, end, hit, miss);
		};

		return true;
	}

	// adapted from https://tavianator.com/2015/ray_box_nan.html
	bool rayIntersectsAABB(glm::vec3 bmin, glm::vec3 bmax, glm::vec3 rayStart, glm::vec3 rayEnd, float& t) 
	{
		const auto invDir = 1.0f / glm::normalize(rayEnd - rayStart);
		double t1 = (bmin[0] - rayStart[0]) * invDir[0];
		double t2 = (bmax[0] - rayStart[0]) * invDir[0];
		double tmin = glm::min(t1, t2);
		double tmax = glm::max(t1, t2);
		for (int i = 1; i < 3; ++i) 
		{
			t1 = (bmin[i] - rayStart[i]) * invDir[i];
			t2 = (bmax[i] - rayStart[i]) * invDir[i];

			tmin = glm::max(tmin, glm::min(glm::min(t1, t2), tmax));
			tmax = glm::min(tmax, glm::max(glm::max(t1, t2), tmin));
		}

		// tmin < 0 = start point inside the box
		t = tmin < 0 ? tmax : tmin;

		return tmax >= glm::max(tmin, 0.0);		// >= - we treat rays on the edge of objects as an intersection
	}

	bool RaycastSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		bool openWin = true;
		m_debugGuiSystem->BeginWindow(openWin, "Raycasts");
		m_debugGuiSystem->Text("Pending Raycasts: %d", m_pendingRays.size());
		m_debugGuiSystem->EndWindow();

		// append all the pending rays to a large ssbo (active ray buffer)
		// (ray-sdf shaders will lookup into this table by index)
		m_activeRays = std::move(m_pendingRays);
		void* activeRayBufferPtr = m_activeRayBuffer->Map(Render::RenderBufferMapHint::Write, 0, m_activeRayBuffer->GetSize());
		float* rayBuff = reinterpret_cast<float*>(activeRayBufferPtr);
		for (const auto& it : m_activeRays)
		{
			rayBuff[0] = it.m_p0.x;
			rayBuff[1] = it.m_p0.y;
			rayBuff[2] = it.m_p0.z;
			rayBuff[3] = it.m_p1.x;
			rayBuff[4] = it.m_p1.y;
			rayBuff[5] = it.m_p1.z;
			rayBuff = rayBuff + 6;
		}
		m_activeRayBuffer->Unmap();

		// broad phase against bounds and collect ray indices to test per sdf
		uint32_t* indexBuffer = (uint32_t*)m_activeRayIndexBuffer->Map(Render::RenderBufferMapHint::Write, 0, m_activeRayIndexBuffer->GetSize());
		m_activeRayIndices.clear();
		uint32_t* currentIndex = indexBuffer;
		auto world = m_entitySystem->GetWorld();
		auto transforms = world->GetAllComponents<Transform>();
		world->ForEachComponent<SDFMesh>([&](SDFMesh& m, EntityHandle owner) {
			const Transform* transform = transforms->Find(owner);
			if (!transform)
				return;
			// transform ray to object space for aabb intersection
			auto inverseTransform = glm::inverse(transform->GetMatrix());
			std::vector<uint32_t> raysToCast;	// indices into active ray buffer
			for (const auto& r : m_activeRays)
			{
				const auto rs = glm::vec3(inverseTransform * glm::vec4(r.m_p0, 1));
				const auto re = glm::vec3(inverseTransform * glm::vec4(r.m_p1, 1));
				float tPoint = 0.0f;
				if (rayIntersectsAABB(m.GetBoundsMin(), m.GetBoundsMax(), rs, re, tPoint))
				{
					raysToCast.push_back(&r - m_activeRays.data());
				}
			}
			if (raysToCast.size() > 0)
			{
				// push the active ray indices to the gpu buffer
				auto startIndexOffset = currentIndex - indexBuffer;
				for (const auto& it : raysToCast)
				{
					*currentIndex++ = it;
					m_activeRayIndices.push_back(it);
				}

				// the data we just wrote is to be used as a ssbo
				// may need some kind of flush for mapped buffer, not sure
				m_renderSys->GetDevice()->MemoryBarrier(Render::BarrierType::ShaderStorage);

				Engine::ShaderManager::CustomDefines shaderDefines = { {"SDF_SHADER_INCLUDE", m.GetSDFShaderPath()} };
				const auto shaderName = "SDF Raycast " + m.GetSDFShaderPath();
				auto loadedShader = m_graphics->Shaders().LoadComputeShader(shaderName.c_str(), "sdf_raycast.cs", shaderDefines);
				auto raycastShader = m_graphics->Shaders().GetShader(loadedShader);

				// call raycast shader, passing ray indices, ray buffer, and start offset
				// the shader will write for each ray index, normalAtIntersection, tDistance (<0 for no hit) as a vec4
				auto device = m_renderSys->GetDevice();
				device->BindShaderProgram(*raycastShader);
				device->BindStorageBuffer(0, *m_activeRayBuffer);
				device->BindStorageBuffer(1, *m_activeRayIndexBuffer);
				device->BindStorageBuffer(2, *m_raycastOutputBuffer);
				auto handle = raycastShader->GetUniformHandle("RayIndexOffset");
				if(handle!=-1)	device->SetUniformValue(handle, (uint32_t)startIndexOffset);
				handle = raycastShader->GetUniformHandle("RayCount");
				if(handle!=-1)	device->SetUniformValue(handle, (uint32_t)raysToCast.size());
				handle = raycastShader->GetUniformHandle("EntityID");
				if(handle!=-1)	device->SetUniformValue(handle, owner.GetID());
				Render::Material* instanceMaterial = nullptr;
				auto matComponent = m_entitySystem->GetWorld()->GetComponent<Material>(m.GetMaterialEntity());
				if (matComponent != nullptr)
				{
					instanceMaterial = &matComponent->GetRenderMaterial();
					if (instanceMaterial)	// if we have a custom material, send the params!
					{
						ApplyMaterial(*device, *raycastShader, *instanceMaterial, m_graphics->Textures());
					}
				}
				uint32_t dispatchCount = ((raysToCast.size() - 1) / 64 + 1) * 64;
				device->DispatchCompute(dispatchCount, 1, 1);
			}
		});
		m_activeRayIndexBuffer->Unmap();

		if (m_activeRays.size() > 0)
		{
			m_activeRayFence = m_renderSys->GetDevice()->MakeFence();
		}

		return true;
	}

	void RaycastSystem::Shutdown()
	{
		SDE_PROF_EVENT();
		m_pendingRays.clear();
		m_activeRays.clear();
		m_raycastOutputBuffer = nullptr;
		m_activeRayBuffer = nullptr;
		m_activeRayIndexBuffer = nullptr;
	}
}