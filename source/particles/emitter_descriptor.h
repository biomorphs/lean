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

		std::string_view GetName() const { return m_name; }
		void SetName(std::string_view n) { m_name = n; }

		SERIALISED_CLASS();

		using EmissionBehaviours = std::vector<std::unique_ptr<EmissionBehaviour>>;
		EmissionBehaviours& GetEmissionBehaviours() { return m_emissionBehaviours; }
	private:
		std::string m_name;
		EmissionBehaviours m_emissionBehaviours;
	};
}