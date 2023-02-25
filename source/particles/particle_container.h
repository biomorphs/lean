#pragma once

#include "particle_buffer.h"
#include "core/glm_headers.h"

namespace Particles
{
	class ParticleContainer
	{
	public:
		ParticleContainer();
		ParticleContainer(uint32_t maxParticles);
		~ParticleContainer();

		void Create(uint32_t maxParticles);

		typedef __m128 PositionType;
		typedef __m128 VelocityType;
		typedef __m128 ColourType;
		typedef float LifetimeType;

		inline const ParticleBuffer<PositionType>& Positions() const { return m_position; }
		inline ParticleBuffer<PositionType>& Positions() { return m_position; }
		inline const ParticleBuffer<VelocityType>& Velocities() const { return m_velocity; }
		inline ParticleBuffer<VelocityType>& Velocities() { return m_velocity; }
		inline const ParticleBuffer<ColourType>& Colours() const { return m_colour; }
		inline ParticleBuffer<ColourType>& Colours() { return m_colour; }
		inline const ParticleBuffer<LifetimeType>& Lifetimes() const { return m_lifetime; }
		inline ParticleBuffer<LifetimeType>& Lifetimes() { return m_lifetime; }

		uint32_t Wake(uint32_t count);
		void Kill(uint32_t index);
		inline uint32_t MaxParticles() const { return m_maxParticles; }
		inline uint32_t AliveParticles() const { return m_livingParticles; }
		inline size_t ParticleSizeBytes() const;

	private:
		uint32_t m_maxParticles;
		uint32_t m_livingParticles;

		ParticleBuffer<PositionType> m_position;
		ParticleBuffer<VelocityType> m_velocity;
		ParticleBuffer<ColourType> m_colour;
		ParticleBuffer<LifetimeType> m_lifetime;
	};
}
#include "particle_container.inl"