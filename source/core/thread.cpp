#pragma once
#include "thread.h"
#include <cassert>
#include <SDL_thread.h>
#include <SDL_timer.h>

namespace Core
{
	Thread::Thread()
		: m_handle(nullptr)
	{
	}

	Thread::~Thread()
	{
		WaitForFinish();
	}

	void Thread::Sleep(int ms)
	{
		SDL_Delay(ms);
	}

	int32_t Thread::ThreadFn(void *ptr)
	{
		auto t = static_cast<Thread*>(ptr);
		assert(t != nullptr);

		return t->m_function();
	}

	void Thread::Create(const char* name, TheadFunction fn)
	{
		assert(m_handle == nullptr);
		m_function = fn;
		m_handle = SDL_CreateThread(ThreadFn, name, this);
		assert(m_handle != nullptr);
	}

	int32_t Thread::WaitForFinish()
	{
		if (m_handle != nullptr)
		{
			int32_t result = 0;
			SDL_WaitThread(static_cast<SDL_Thread*>(m_handle), &result);
			m_handle = nullptr;
			return result;
		}
		else
		{
			return 0;
		}
	}
}
