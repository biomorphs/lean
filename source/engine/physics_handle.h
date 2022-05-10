#pragma once

#include <memory>

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
		inline explicit PhysicsHandle(T* ptr) { m_v = ptr; }
		PhysicsHandle& operator=(T* ptr);
		~PhysicsHandle();
		PhysicsHandle(const PhysicsHandle& other) = delete;
		inline PhysicsHandle& operator=(const PhysicsHandle& other) = delete;
		PhysicsHandle(PhysicsHandle&& other);
		PhysicsHandle& operator=(PhysicsHandle&& other);

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