#include "emit_once.h"
#include "particles/editor_value_inspector.h"

namespace Particles
{
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