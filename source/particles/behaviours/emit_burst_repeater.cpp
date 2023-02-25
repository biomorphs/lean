#include "emit_burst_repeater.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(EmitBurstRepeater, EmissionBehaviour)
	{
		SERIALISE_PROPERTY("Burst Count", m_burstCount);
		SERIALISE_PROPERTY("Frequency", m_frequency);
	}
	SERIALISE_END()

	void EmitBurstRepeater::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Burst Count", m_burstCount, [this](int v) {
			m_burstCount = v;
		});
		v.Inspect("Frequency", m_frequency, [this](float v) {
			m_frequency = v;
		});
	}

	int EmitBurstRepeater::Emit(double emitterAge, float deltaTime)
	{
		SDE_PROF_EVENT();
		float v = fmod(emitterAge, m_frequency);
		if (v > -0.01f && v < 0.01f)
		{
			return m_burstCount;
		}
		return 0;
	}
}