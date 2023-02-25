#pragma once
#include "core/glm_headers.h"
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	// needs to go After position is set!
	class GenerateSpawnEmitter : public GeneratorBehaviour
	{
	public:
		SERIALISED_CLASS();
		void Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
		std::unique_ptr<GeneratorBehaviour> MakeNew() { return std::make_unique<GenerateSpawnEmitter>(); }
		std::string_view GetName() { return "Spawn Emitter"; }
		void Inspect(EditorValueInspector& v);

		std::string m_emitterFile;
	};
}