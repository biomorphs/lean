#pragma once

#include <functional>
#include <memory>

namespace Particles
{
	template<class ValueType>
	class ParticleBuffer
	{
	public:
		ParticleBuffer();
		ParticleBuffer(uint32_t maxValues);
		~ParticleBuffer();

		enum
		{
			DataSize = sizeof(ValueType)
		};

		void Create(uint32_t maxValues, const ValueType* defaultValue = nullptr);
		uint32_t Wake(uint32_t count, const ValueType* defaultValue = nullptr);
		void Kill(uint32_t index);
		void SetValue(uint32_t index, const ValueType& t);
		ValueType& GetValue(uint32_t index);
		const ValueType& GetValue(uint32_t index) const;
		inline const uint32_t AliveCount() const { return m_aliveCount; }

	private:
		std::unique_ptr<ValueType, std::function<void(ValueType*)>> m_dataBuffer;
		uint32_t m_maxValues;
		uint32_t m_aliveCount;
	};
}

#include "particle_buffer.inl"