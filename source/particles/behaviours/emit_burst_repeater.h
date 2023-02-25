#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class ValueInspector;
	class EmitBurstRepeater : public EmissionBehaviour
	{
	public:
		SERIALISED_CLASS();
		virtual int Emit(double emitterAge, float deltaTime);
		std::unique_ptr<EmissionBehaviour> MakeNew() { return std::make_unique<EmitBurstRepeater>(); }
		std::string_view GetName() { return "Burst Repeater"; }
		void Inspect(EditorValueInspector& v);

		int m_burstCount = 1;
		float m_frequency = 0.5f;
	};
}