#pragma once
#include "system.h"
#include "core/glm_headers.h"
#include "entity/entity_handle.h"
#include "render/fence.h"
#include <functional>

namespace Render
{
	class RenderBuffer;
}

class GraphicsSystem;
class EntitySystem;
namespace Engine
{
	class JobSystem;
	class DebugGuiSystem;
	class ScriptSystem;
	class RenderSystem;
	class PhysicsSystem;
	class RaycastSystem : public System
	{
	public:
		RaycastSystem();
		virtual ~RaycastSystem();

		bool PreInit(SystemManager& manager);
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();

		// start, end, hitpos, normal, entity
		using RayHitFn = std::function<void(glm::vec3, glm::vec3, glm::vec3,glm::vec3,EntityHandle)>;
		using RayMissFn = std::function<void(glm::vec3, glm::vec3)>;	// start, end
		void RaycastAsync(glm::vec3 p0, glm::vec3 p1, RayHitFn onHit, RayMissFn onMiss);

		class ProcessResults : public System
		{
		public:
			ProcessResults(RaycastSystem* p) : m_parent(p) {}
			bool Tick(float timeDelta);
		private:
			RaycastSystem* m_parent = nullptr;
		};
		ProcessResults* MakeResultProcessor();

	private:
		struct Raycast {
			glm::vec3 m_p0;
			glm::vec3 m_p1;
			RayHitFn m_onHit;
			RayMissFn m_onMiss;
		};
		std::vector<Raycast> m_pendingRays;			// these havent started yet
		std::vector<Raycast> m_activeRays;			// these are in flight
		std::vector<uint32_t> m_activeRayIndices;	// each shader invocation uses a subset of indices
		std::unique_ptr<Render::RenderBuffer> m_raycastOutputBuffer;
		std::unique_ptr<Render::RenderBuffer> m_activeRayBuffer;
		std::unique_ptr<Render::RenderBuffer> m_activeRayIndexBuffer;
		Render::Fence m_activeRayFence;
		JobSystem* m_jobSystem = nullptr;
		EntitySystem* m_entitySystem = nullptr;
		DebugGuiSystem* m_debugGuiSystem = nullptr;
		ScriptSystem* m_scriptSystem = nullptr;
		RenderSystem* m_renderSys = nullptr;
		GraphicsSystem* m_graphics = nullptr;
		PhysicsSystem* m_physics = nullptr;
	};
}
