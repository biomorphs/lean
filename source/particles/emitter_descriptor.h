#pragma once
#include "engine/serialisation.h"
#include "particle_emission_behaviour.h"

namespace Particles
{
	class EmitterDescriptor
	{
	public:
		EmitterDescriptor();
		~EmitterDescriptor();

		void Reset();

		std::string_view GetName() const { return m_name; }
		void SetName(std::string_view n) { m_name = n; }

		int GetMaxParticles() const { return m_maxParticles; }
		void SetMaxParticles(int v) { m_maxParticles = v; }

		SERIALISED_CLASS();

		using EmissionBehaviours = std::vector<std::unique_ptr<EmissionBehaviour>>;
		EmissionBehaviours& GetEmissionBehaviours() { return m_emissionBehaviours; }

		using GeneratorBehaviours = std::vector<std::unique_ptr<GeneratorBehaviour>>;
		GeneratorBehaviours& GetGenerators() { return m_generatorBehaviours; }

		using UpdateBehaviours = std::vector<std::unique_ptr<UpdateBehaviour>>;
		UpdateBehaviours& GetUpdaters() { return m_updateBehaviours; }

		using RenderBehaviours = std::vector<std::unique_ptr<RenderBehaviour>>;
		RenderBehaviours& GetRenderers() { return m_renderBehaviours; }

	private:
		std::string m_name;
		int m_maxParticles = 256;
		EmissionBehaviours m_emissionBehaviours;
		GeneratorBehaviours m_generatorBehaviours;
		UpdateBehaviours m_updateBehaviours;
		RenderBehaviours m_renderBehaviours;
	};
}