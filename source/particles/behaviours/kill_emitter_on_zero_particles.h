#pragma once
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class KillOnZeroParticles : public EmitterLifetimeBehaviour
	{
	public:
		SERIALISED_CLASS();
		std::unique_ptr<EmitterLifetimeBehaviour> MakeNew() { return std::make_unique<KillOnZeroParticles>(); }
		std::string_view GetName() { return "Kill on zero particles"; }

		bool ShouldStop(double emitterAge, float deltaTime, ParticleContainer& container);
		void Inspect(EditorValueInspector&) {}
	};
}