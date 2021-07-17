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

		struct RayHitResult {
			glm::vec3 m_start;
			glm::vec3 m_end;
			glm::vec3 m_hitPos;
			glm::vec3 m_hitNormal;
			EntityHandle m_hitEntity;
		};
		struct RayMissResult {
			glm::vec3 m_start;
			glm::vec3 m_end;
		};
		struct RayInput {
			RayInput() = default;
			RayInput(glm::vec3 start, glm::vec3 end) : m_start(start), m_end(end) {}
			glm::vec3 m_start;
			glm::vec3 m_end;
		};

		using RayResultsFn = std::function<void(const std::vector<RayHitResult>&, const std::vector<RayMissResult>&)>;
		void RaycastAsyncMulti(const std::vector<RayInput>& rays, const RayResultsFn& fn);

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
		struct RaycastRequest {		// input rays are pushed to m_activeRays, this tracks them for later
			uint32_t m_firstRay;	// index into m_activeRays
			uint32_t m_rayCount;	// count for above
			RayResultsFn m_resultFn;
		};
		std::vector<RaycastRequest> m_pendingRequests;	// these havent started yet
		std::vector<RayInput> m_pendingRays;			// ^^
		std::vector<RaycastRequest> m_activeRequests;	// these are in flight
		std::vector<RayInput> m_activeRays;				// ^^
		std::vector<uint32_t> m_activeRayIndices;		// each shader invocation uses a subset of indices
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
