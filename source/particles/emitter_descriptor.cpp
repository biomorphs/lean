#include "emitter_descriptor.h"

namespace Particles
{
	EmitterDescriptor::EmitterDescriptor()
	{

	}

	EmitterDescriptor::~EmitterDescriptor()
	{

	}

	void EmitterDescriptor::Reset()
	{
		m_name = "";
		m_maxParticles = 256;
		m_emissionBehaviours.clear();
		m_generatorBehaviours.clear();
		m_updateBehaviours.clear();
		m_renderBehaviours.clear();
		m_lifetimeBehaviours.clear();
	}

	SERIALISE_BEGIN(EmitterDescriptor)
	{
		SERIALISE_PROPERTY("Name", m_name);
		SERIALISE_PROPERTY("MaxParticles", m_maxParticles);
		SERIALISE_PROPERTY("OwnsChildEmitters", m_ownsChildEmitters)
		SERIALISE_PROPERTY("EmissionBehaviours", m_emissionBehaviours);
		SERIALISE_PROPERTY("GeneratorBehaviours", m_generatorBehaviours);
		SERIALISE_PROPERTY("UpdateBehaviours", m_updateBehaviours);
		SERIALISE_PROPERTY("RenderBehaviours", m_renderBehaviours);
		SERIALISE_PROPERTY("LifetimeBehaviours", m_lifetimeBehaviours);
	}
	SERIALISE_END()
}