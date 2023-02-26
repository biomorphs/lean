#include "kill_emitter_on_zero_particles.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(KillOnZeroParticles, EmitterLifetimeBehaviour)
	{
	}
	SERIALISE_END();

	bool KillOnZeroParticles::ShouldStop(double emitterAge, float deltaTime, ParticleContainer& container)
	{
		// dont kill on first update (allows frame 0 burst)
		return emitterAge > 0.0 && container.AliveParticles() == 0;
	}
}