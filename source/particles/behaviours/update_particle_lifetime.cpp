#include "update_particle_lifetime.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "engine/components/component_environment_settings.h"
#include "entity/entity_system.h"
#include "engine/system_manager.h"
#include "particles/particle_system.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(UpdateParticleLifetime, UpdateBehaviour)
	{
		SERIALISE_PROPERTY("KillAttachedEmitters", m_killAttachedEmitters);
	}
	SERIALISE_END();

	void UpdateParticleLifetime::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Kill attached emitter on particle death", m_killAttachedEmitters, [this](bool v) {
			m_killAttachedEmitters = v;
		});
	}

	void UpdateParticleLifetime::Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		static auto particles = Engine::GetSystem<ParticleSystem>("Particles");
		uint32_t currentP = 0;
		while (currentP < container.AliveParticles())
		{
			float currentAge = emitterAge - container.SpawnTimes().GetValue(currentP);
			float maxLifetime = container.Lifetimes().GetValue(currentP);
			if (currentAge >= maxLifetime)
			{
				if (m_killAttachedEmitters)
				{
					uint32_t attachedEmitter = container.EmitterIDs().GetValue(currentP);
					if (attachedEmitter != -1)
					{
						particles->StopEmitter(attachedEmitter);
					}
				}
				container.Kill(currentP);
			}
			else
			{
				currentP++;
			}
		}
	}
}