#include "update_attached_emitter.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "engine/components/component_environment_settings.h"
#include "entity/entity_system.h"
#include "engine/system_manager.h"
#include "particles/particle_system.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(UpdateAttachedEmitter, UpdateBehaviour)
	{
	}
	SERIALISE_END();

	void UpdateAttachedEmitter::Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		static auto particles = Engine::GetSystem<ParticleSystem>("Particles");
		const uint32_t endIndex = container.AliveParticles();
		__declspec(align(16)) glm::vec4 particlePos;
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			uint32_t emitterId = container.EmitterIDs().GetValue(i);
			if (emitterId != -1)
			{
				_mm_store_ps(glm::value_ptr(particlePos), container.Positions().GetValue(i));
				particles->SetEmitterTransform(emitterId, glm::vec3(particlePos));
			}
		}
		_mm_sfence();
	}
}