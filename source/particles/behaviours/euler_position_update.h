#pragma once
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class EulerPositionUpdater : public UpdateBehaviour
	{
	public:
		SERIALISED_CLASS();
		void Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container);
		std::unique_ptr<UpdateBehaviour> MakeNew() { return std::make_unique<EulerPositionUpdater>(); }
		std::string_view GetName() { return "Euler Position Updater"; }
		void Inspect(EditorValueInspector&) {}
	};
}