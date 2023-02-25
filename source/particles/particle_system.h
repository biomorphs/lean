#pragma once 
#include "engine/system.h"
#include "core/glm_headers.h"
#include "emitter_descriptor.h"
#include <string_view>
#include <unordered_map>
#include <memory>

namespace Particles
{
	class EmitterInstance;
	class ParticleSystem : public Engine::System
	{
	public:
		ParticleSystem();
		virtual ~ParticleSystem();
		void InvalidateEmitter(std::string_view filename);	// force reload
		uint64_t StartEmitter(std::string_view filename, glm::vec3 pos = glm::vec3(0, 0, 0), glm::quat rot = glm::quat());
		virtual bool Tick(float timeDelta);
	private:
		struct ActiveEmitter {
			uint64_t m_id;
			EmitterInstance* m_instance;
		};
		void UpdateActiveInstance(ActiveEmitter& em, float timeDelta);
		bool LoadEmitter(std::string_view path, EmitterDescriptor& result);

		std::unordered_map<std::string, std::unique_ptr<EmitterDescriptor>> m_loadedEmitters;
		bool m_updateEmitters = true;
		bool m_renderEmitters = true;
		std::vector<ActiveEmitter> m_activeEmitters;
		uint64_t m_nextEmitterId = 0;
	};
}