#pragma once
#include "core/glm_headers.h"
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class GenerateRandomPosition : public GeneratorBehaviour
	{
	public:
		SERIALISED_CLASS();
		void Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
		std::unique_ptr<GeneratorBehaviour> MakeNew() { return std::make_unique<GenerateRandomPosition>(); }
		std::string_view GetName() { return "Random Position"; }
		void Inspect(EditorValueInspector& v);

		glm::vec3 m_boundsMin = { 0,0,0 };
		glm::vec3 m_boundsMax = { 1,1,1 };
	};
}