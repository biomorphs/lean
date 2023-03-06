#pragma once
#include "core/glm_headers.h"
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class GenerateRandomVelocitySphere : public GeneratorBehaviour
	{
	public:
		SERIALISED_CLASS();
		void Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
		std::unique_ptr<GeneratorBehaviour> MakeNew() { return std::make_unique<GenerateRandomVelocitySphere>(); }
		std::string_view GetName() { return "Random Velocity (Sphere)"; }
		void Inspect(EditorValueInspector& v);

		float m_minMagnitude = 0.0f;
		float m_maxMagnitude = 1.0f;
		float m_minTheta = 0.0f;
		float m_maxTheta = glm::pi<float>() * 2.0f;
		float m_minPhi = 0.0f;
		float m_maxPhi = glm::pi<float>() * 2.0f;
	};
}