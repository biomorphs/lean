#pragma once
#include "core/glm_headers.h"
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class GenerateRandomLifetime : public GeneratorBehaviour
	{
	public:
		SERIALISED_CLASS();
		void Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
		std::unique_ptr<GeneratorBehaviour> MakeNew() { return std::make_unique<GenerateRandomLifetime>(); }
		std::string_view GetName() { return "Random Particle Lifetime"; }
		void Inspect(EditorValueInspector& v);

		float m_minLifetime = 0.0f;
		float m_maxLifetime = 1.0f;
	};
}