#include "semaphore.h"
#include <cassert>
#include <SDL_mutex.h>

namespace Core
{
	Semaphore::Semaphore(uint32_t initialValue)
	{
		m_semaphore = SDL_CreateSemaphore(initialValue);
		assert(m_semaphore);
	}

	Semaphore::~Semaphore()
	{
		SDL_DestroySemaphore(static_cast<SDL_semaphore*>(m_semaphore));
		m_semaphore = nullptr;
	}

	void Semaphore::Post()
	{
		int32_t result = SDL_SemPost(static_cast<SDL_semaphore*>(m_semaphore));
		assert(result == 0);
	}

	void Semaphore::Wait()
	{
		int32_t result = SDL_SemWait(static_cast<SDL_semaphore*>(m_semaphore));
		assert(result == 0);
	}
}