#pragma once
#include <stdint.h>

namespace Core
{
	class Semaphore
	{
	public:
		Semaphore(uint32_t initialValue);
		~Semaphore();

		void Post();
		void Wait();

	private:
		void* m_semaphore;
	};
}