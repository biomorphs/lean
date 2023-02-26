#pragma once 
#include "engine/system.h"
#include "core/glm_headers.h"
#include "core/mutex.h"
#include "emitter_descriptor.h"
#include <robin_hood.h>
#include <string_view>
#include <memory>

namespace Particles
{
	class EmitterInstance;
	class ParticleSystem : public Engine::System
	{
	public:
		ParticleSystem();
		virtual ~ParticleSystem();
		using EmitterID = uint32_t;
		void InvalidateEmitter(std::string_view filename);	// force reload
		EmitterID StartEmitter(std::string_view filename, glm::vec3 pos = glm::vec3(0, 0, 0), glm::quat rot = glm::quat());
		void StopEmitter(EmitterID emitterID);
		void SetEmitterTransform(EmitterID emitterID, glm::vec3 pos = glm::vec3(0, 0, 0), glm::quat rot = glm::quat());
		virtual bool Tick(float timeDelta);
	private:
		struct ActiveEmitter {
			uint32_t m_id;
			EmitterInstance* m_instance;
		};
		void UpdateActiveInstance(ActiveEmitter& em, float timeDelta);
		bool LoadEmitter(std::string_view path, EmitterDescriptor& result);
		void ShowStats();
		void StartNewEmitters();
		void UpdateEmitters(float timeDelta);
		void StopEmitters();
		void RenderEmitters(float timeDelta);
		void ReloadInvalidatedEmitters();
		void DoStopEmitter(EmitterInstance& i);

		Core::Mutex m_loadedEmittersMutex;
		std::unordered_map<std::string, std::unique_ptr<EmitterDescriptor>> m_loadedEmitters;
		bool m_updateEmitters = true;
		bool m_renderEmitters = true;
		bool m_showStats = true;
		robin_hood::unordered_map<EmitterID, uint32_t> m_activeEmitterIDToIndex;
		std::vector<ActiveEmitter> m_activeEmitters;
		Core::Mutex m_startEmittersMutex;
		std::vector<ActiveEmitter> m_emittersToStart;
		Core::Mutex m_stopEmittersMutex;
		std::vector<EmitterID> m_emittersToStop;
		Core::Mutex m_invalidatedEmitterMutex;
		std::vector<std::string> m_invalidatedEmitters;
		int m_maxEmitters = 1024 * 64;
		std::atomic<EmitterID> m_nextEmitterId = 0;
	};
}