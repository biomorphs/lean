#pragma once
#include "core/glm_headers.h"
#include "particle_container.h"

namespace Particles
{
	class EmitterDescriptor;
	class EmitterInstance
	{
	public:
		ParticleContainer m_particles;
		EmitterDescriptor* m_emitter;
		double m_timeActive = 0.0;
		glm::vec3 m_position;
		glm::quat m_orientation;
	};
}