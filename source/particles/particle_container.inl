namespace Particles
{
	inline ParticleContainer::ParticleContainer(uint32_t maxParticles)
	{
		Create(maxParticles);
	}

	inline ParticleContainer::ParticleContainer()
		: m_maxParticles(0)
		, m_livingParticles(0)
	{
	}

	inline ParticleContainer::~ParticleContainer()
	{
	}

	inline size_t ParticleContainer::ParticleSizeBytes() const
	{
		return m_position.DataSize + m_lifetime.DataSize + m_velocity.DataSize;
	}

	inline void ParticleContainer::Create(uint32_t maxParticles)
	{
		m_maxParticles = maxParticles;
		m_livingParticles = 0;

		m_position.Create(maxParticles);
		m_lifetime.Create(maxParticles);
		m_velocity.Create(maxParticles);
		m_colour.Create(maxParticles);
	}

	inline uint32_t ParticleContainer::Wake(uint32_t count)
	{
		assert(m_livingParticles + count <= m_maxParticles);
		const uint32_t newIndex = m_livingParticles;

		const uint32_t pIndex = m_position.Wake(count);
		assert(pIndex == newIndex);

		const uint32_t lIndex = m_lifetime.Wake(count);
		assert(lIndex == newIndex);

		const uint32_t vIndex = m_velocity.Wake(count);
		assert(vIndex == newIndex);

		const uint32_t cIndex = m_colour.Wake(count);
		assert(cIndex == newIndex);

		m_livingParticles += count;

		return newIndex;
	}

	inline void ParticleContainer::Kill(uint32_t index)
	{
		assert(index < m_maxParticles);
		if (index < m_livingParticles)
		{
			m_position.Kill(index);
			m_lifetime.Kill(index);
			m_velocity.Kill(index);
			m_colour.Kill(index);

			--m_livingParticles;
		}
	}
}