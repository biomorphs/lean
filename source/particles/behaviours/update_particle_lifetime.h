#pragma once
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class UpdateParticleLifetime : public UpdateBehaviour
	{
	public:
		SERIALISED_CLASS();
		std::unique_ptr<UpdateBehaviour> MakeNew() { return std::make_unique<UpdateParticleLifetime>(); }
		std::string_view GetName() { return "Update particle lifetime"; }

		void Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container);
		void Inspect(EditorValueInspector&) {}
	};
}