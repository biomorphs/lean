#include "new_render.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_model.h"
#include "entity/entity_system.h"
#include "engine/frustum.h"
#include "render/vertex_array.h"
#include "core/mutex.h"
#include <atomic>

const uint32_t c_maxInstances = 100000;

uint64_t MakeSortKey(const Engine::ShaderHandle& shader, const Render::VertexArray& va)
{
	const void* vaPtrVoid = static_cast<const void*>(&va);
	const uintptr_t vaPtr = reinterpret_cast<uintptr_t>(vaPtrVoid);
	const uint32_t vaPtrLow = static_cast<uint32_t>((vaPtr & 0x00000000ffffffff));
	return (uint64_t)shader.m_index << 32 | (uint64_t)vaPtrLow;
}

namespace Engine
{
	struct MeshPartInstance
	{
		uint64_t m_sortKey;
		const Render::VertexArray* m_va;
		const Render::ShaderProgram* m_shader;
		const Render::MeshChunk* m_chunks;
		uint64_t m_chunkCount;
		glm::mat4 m_transform;
		glm::vec3 m_boundsMin;
		glm::vec3 m_boundsMax;
	};

	// everything you need to draw a bunch of stuff quickly
	class RenderInstanceData
	{
	public:
		using Completion = std::function<void(RenderInstanceData&)>;
		void FindVisibleInstancesAsync(const Frustum& f, Completion onComplete)
		{
			SDE_PROF_EVENT();
			assert(m_cullJobsRemaining == 0);

			auto jobs = Engine::GetSystem<JobSystem>("Jobs");
			const uint32_t partsPerJob = 4000;
			const uint32_t jobCount = m_allInstances.size() / partsPerJob;
			m_cullJobsRemaining = jobCount;
			if (jobCount == 0)
			{
				onComplete(*this);
			}
			for (uint32_t j = 0; j < jobCount; ++j)
			{
				auto cullPartsJob = [this, j, partsPerJob, f, onComplete](void*) {
					SDE_PROF_EVENT("Cull instances");
					uint32_t firstIndex = j * partsPerJob;
					uint32_t lastIndex = std::min((uint32_t)m_allInstances.size(), firstIndex + partsPerJob);
					std::vector<MeshPartInstance> localResults;	// collect results for this job
					localResults.reserve(partsPerJob);
					for (uint32_t id = firstIndex; id < lastIndex; ++id)
					{
						const auto& theInstance = m_allInstances[id];
						if (f.IsBoxVisible(theInstance.m_boundsMin, theInstance.m_boundsMax, theInstance.m_transform))
						{
							localResults.emplace_back(theInstance);
						}
					}
					if (localResults.size() > 0)	// copy the results to the main list
					{
						SDE_PROF_EVENT("Push results");
						Core::ScopedMutex lock(m_visibleInstanceLock);
						m_visibleInstances.insert(m_visibleInstances.end(), localResults.begin(), localResults.end());
					}
					if (m_cullJobsRemaining.fetch_sub(1) == 1)		// this was the last culling job, time to sort!
					{
						SortVisibleInstancesAsync(onComplete);
					}
				};
				jobs->PushJob(cullPartsJob);
			}
		}

		inline void PushInstance(uint64_t sortKey, const Render::VertexArray* va, const Render::ShaderProgram* shader,
			const Render::MeshChunk* chunks, uint64_t chunkCount,
			glm::mat4 transform, glm::vec3 boundsMin, glm::vec3 boundsMax)
		{
			assert(m_cullJobsRemaining == 0);
			m_allInstances.push_back({ sortKey, va, shader, chunks, chunkCount, transform, boundsMin, boundsMax });
		}

		RenderInstanceData()
		{
			m_allInstances.reserve(c_maxInstances);
			m_visibleInstances.reserve(c_maxInstances);
		}

	private:
		void SortVisibleInstancesAsync(Completion onComplete)
		{
			assert(m_cullJobsRemaining == 0);
			if (m_visibleInstances.size() > 0)
			{
				auto sortingJob = [this, onComplete](void*)
				{
					SDE_PROF_EVENT("SortVisibleParts");
					{
						Core::ScopedMutex lock(m_visibleInstanceLock);
						std::sort(m_visibleInstances.begin(), m_visibleInstances.end(),
							[](const MeshPartInstance& s1, const MeshPartInstance& s2) {
								return s1.m_sortKey < s2.m_sortKey;
						});
					}
					onComplete(*this);
				};
				auto jobs = Engine::GetSystem<JobSystem>("Jobs");
				jobs->PushJob(sortingJob);
			}
			else
			{
				onComplete(*this);
			}
		}
		std::vector<MeshPartInstance> m_allInstances;

		std::atomic<uint32_t> m_cullJobsRemaining = 0;
		Core::Mutex m_visibleInstanceLock;
		std::vector<MeshPartInstance> m_visibleInstances;
	};

	NewRender::NewRender()
	{
		m_transforms.Create(c_maxInstances * sizeof(glm::mat4), Render::RenderBufferModification::Dynamic, true);
	}

	void NewRender::Reset()
	{
		m_transformsWritten = 0;
	}

	void NewRender::RenderAll(class Render::Device&)
	{

	}

	void EnumerateAllInstancesAsync(RenderInstanceData& rid, RenderInstanceData::Completion onComplete)
	{
		SDE_PROF_EVENT();

		auto jobs = Engine::GetSystem<JobSystem>("Jobs");
		auto enumerateMeshesJob = [&, onComplete](void*) {
			SDE_PROF_EVENT("Enumerate mesh chunks");
			auto entities = Engine::GetSystem<EntitySystem>("Entities");
			auto world = entities->GetWorld();
			auto mm = Engine::GetSystem<ModelManager>("Models");
			auto sm = Engine::GetSystem<Engine::ShaderManager>("Shaders");
			static auto modelIterator = world->MakeIterator<::Model, Transform>();
			modelIterator.ForEach([&](::Model& m, Transform& t, EntityHandle h) {
				const auto theModel = mm->GetModel(m.GetModel());
				const auto theShader = sm->GetShader(m.GetShader());
				if (theModel && theShader)
				{
					const auto entityTransform = t.GetWorldspaceMatrix();
					for (const auto& p : theModel->Parts())
					{
						const auto partTransform = entityTransform * p.m_transform;
						const Render::VertexArray* va = &p.m_mesh->GetVertexArray();	// va should be per *model*, not per part!
						const uint64_t sortKey = MakeSortKey(m.GetShader(), p.m_mesh->GetVertexArray());
						rid.PushInstance(sortKey, va, theShader, &p.m_mesh->GetChunks()[0], p.m_mesh->GetChunks().size(), partTransform, p.m_boundsMin, p.m_boundsMax);
					}
				}
			});
			onComplete(rid);
		};
		jobs->PushJob(enumerateMeshesJob);
	}

	void NewRender::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		std::atomic<bool> cullComplete = false;
		auto onCullingComplete = [&](RenderInstanceData& rid) {
			SDE_PROF_EVENT("OnCullingComplete");
			cullComplete = true;
		};

		auto onInstancesReady = [&](RenderInstanceData& rid) {
			SDE_PROF_EVENT("OnInstancesReady");
			Frustum viewFrustum(m_camera.ProjectionMatrix() * m_camera.ViewMatrix());
			rid.FindVisibleInstancesAsync(viewFrustum, onCullingComplete);
		};

		// first we need to collect *all* the mesh instances that we care about
		RenderInstanceData rid;
		EnumerateAllInstancesAsync(rid, onInstancesReady);

		{
			SDE_PROF_STALL("Wait for results");
			auto jobs = Engine::GetSystem<JobSystem>("Jobs");
			while (!cullComplete)
			{
				Core::Thread::Sleep(0);
			}
		}
	}
}