#pragma once
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class UpdateSpawnEmitter : public UpdateBehaviour
	{
	public:
		SERIALISED_CLASS();
		std::unique_ptr<UpdateBehaviour> MakeNew() { return std::make_unique<UpdateSpawnEmitter>(); }
		std::string_view GetName() { return "Spawn emitter repeater"; }

		void Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container);
		void Inspect(EditorValueInspector&);

		std::string m_emitterFile;
		bool m_attachToParticle = false;
		int m_burstCount = 1;
		float m_frequency = 0.5f;
		float m_spawnStartAge = -1;
		float m_spawnEndAge = -1;		// -1 = repeat forever
	};
}