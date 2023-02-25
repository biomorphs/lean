#include "emitter_descriptor.h"

namespace Particles
{
	EmitterDescriptor::EmitterDescriptor()
	{

	}

	EmitterDescriptor::~EmitterDescriptor()
	{

	}

	SERIALISE_BEGIN(EmitterDescriptor)
	{
		SERIALISE_PROPERTY("Name", m_name);
		SERIALISE_PROPERTY("EmissionBehaviours", m_emissionBehaviours);
	}
	SERIALISE_END()
}