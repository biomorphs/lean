#include "emit_once.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	int EmitOnce::Emit(double emitterAge, float deltaTime)
	{
		SDE_PROF_EVENT();
		if (emitterAge == 0.0)
		{
			return m_emissionCount;
		}
		return 0;
	}

	void EmitOnce::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Count", m_emissionCount, [this](int v) {
			m_emissionCount = v;
		});
	}

	SERIALISE_BEGIN_WITH_PARENT(EmitOnce, EmissionBehaviour)
	{
		SERIALISE_PROPERTY("Emission Count", m_emissionCount);
	}
	SERIALISE_END()
}