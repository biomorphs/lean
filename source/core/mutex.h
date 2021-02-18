#pragma once

namespace Core
{
	class Mutex
	{
	public:
		Mutex();
		Mutex(const Mutex& other) = delete;
		Mutex(Mutex&& other);
		~Mutex();

		void Lock();
		void Unlock();

	private:
		void* m_mutex;
	};

	class ScopedMutex
	{
	public:
		ScopedMutex(Mutex& target) : m_mutex(target) { m_mutex.Lock(); }
		~ScopedMutex() { m_mutex.Unlock(); }
	private:
		Mutex& m_mutex;
	};
}