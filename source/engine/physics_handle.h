#pragma once

#include <memory>

#pragma optimize("",off)

namespace Engine
{
	template<class T>
	class PhysicsHandle
	{
	public:
		template <typename T>
		class HasAcquireReference
		{
			typedef char one;
			typedef long two;
			template <typename C> static one test(decltype(&C::acquireReference));
			template <typename C> static two test(...);

		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		PhysicsHandle() = default;
		inline PhysicsHandle(T* ptr) { m_v = ptr; }
		inline ~PhysicsHandle()
		{
			if (m_v)
			{
				m_v->release();
			}
		}
		PhysicsHandle(const PhysicsHandle& other) = delete;
		inline PhysicsHandle& operator=(const PhysicsHandle& other) = delete;
		inline PhysicsHandle(PhysicsHandle&& other)
		{
			m_v = other.m_v;
			other.m_v = nullptr;
		}
		inline PhysicsHandle& operator=(PhysicsHandle&& other)
		{
			if (m_v)
			{
				m_v->release();
			}
			m_v = other.m_v;
			other.m_v = nullptr;
			return *this;
		}
		inline const T* Get() const { return m_v; }
		inline T* Get() { return m_v; }
		inline T* operator->()
		{
			return m_v;
		}
		inline const T* operator->() const
		{
			return m_v;
		}
	private:
		T* m_v = nullptr;
	};
}

#pragma optimize("",on)