#pragma once
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class EmitOnce : public EmissionBehaviour
	{
	public:
		SERIALISED_CLASS();
		std::unique_ptr<EmissionBehaviour> MakeNew() { return std::make_unique<EmitOnce>(); }
		std::string_view GetName() { return "Burst Once"; }
		void Inspect(EditorValueInspector& v);

		int m_emissionCount = 1;
	};
}