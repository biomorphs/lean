namespace Particles
{

	template<class ValueType>
	inline ParticleBuffer<ValueType>::ParticleBuffer(uint32_t maxValues)
		: m_dataBuffer(nullptr)
		, m_maxValues(0)
		, m_aliveCount(0)
	{
		Create(maxValues);
	}

	template<class ValueType>
	inline ParticleBuffer<ValueType>::ParticleBuffer()
		: m_dataBuffer(nullptr)
		, m_maxValues(0)
		, m_aliveCount(0)
	{
	}

	template<class ValueType>
	inline ParticleBuffer<ValueType>::~ParticleBuffer()
	{
		m_dataBuffer = nullptr;
	}

	template<class ValueType>
	inline void ParticleBuffer<ValueType>::Create(uint32_t maxValues, const ValueType* defaultValue)
	{
		void* rawBuffer = _aligned_malloc(maxValues * sizeof(ValueType), 16);
		assert(rawBuffer);

		ValueType* newValues = reinterpret_cast<ValueType*>(rawBuffer);
		if (defaultValue != nullptr)
		{
			for (int i = 0; i < maxValues; ++i)
			{
				newValues[i] = *defaultValue;
			}
		}

		m_maxValues = maxValues;
		m_aliveCount = 0;

		auto deleter = [](ValueType* p)
		{
			_aligned_free(p);
		};
		m_dataBuffer = std::unique_ptr<ValueType, decltype(deleter)>(newValues, deleter);
	}

	template<class ValueType>
	inline uint32_t ParticleBuffer<ValueType>::Wake(uint32_t count, const ValueType* defaultValue)
	{
		assert(m_aliveCount + count <= m_maxValues);
		const uint32_t alive = m_aliveCount;
		m_aliveCount += count;

		if (defaultValue != nullptr)
		{
			const uint32_t lastIndex = m_aliveCount;
			for (int i = alive; i < m_aliveCount; ++i)
			{
				GetValue(i) = *defaultValue;
			}
		}

		return alive;
	}

	template<class ValueType>
	inline void ParticleBuffer<ValueType>::Kill(uint32_t index)
	{
		assert(index < m_aliveCount);
		if (index == (m_aliveCount - 1))
		{
			--m_aliveCount;
		}
		else if (m_aliveCount > 1)
		{
			*(m_dataBuffer.get() + index) = *(m_dataBuffer.get() + m_aliveCount - 1);
			--m_aliveCount;
		}
		else
		{
			----m_aliveCount;
		}
	}

	template<class ValueType>
	inline void ParticleBuffer<ValueType>::SetValue(uint32_t index, const ValueType& t)
	{
		assert(index < m_aliveCount);
		*(m_dataBuffer.get() + index) = t;
	}

	template<class ValueType>
	inline ValueType& ParticleBuffer<ValueType>::GetValue(uint32_t index)
	{
		assert(index < m_aliveCount);
		return *(m_dataBuffer.get() + index);
	}

	template<class ValueType>
	inline const ValueType& ParticleBuffer<ValueType>::GetValue(uint32_t index) const
	{
		assert(index < m_aliveCount);
		return *(m_dataBuffer.get() + index);
	}
}