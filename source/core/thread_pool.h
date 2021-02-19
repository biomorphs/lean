#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <atomic>

namespace Core
{
	// A pool of threads, all of which call the same thread function repeatedly
	// until Stop() is called, or the ThreadPool is destroyed
	class ThreadPool
	{
	public:
		ThreadPool();
		~ThreadPool();

		typedef std::function<void(uint32_t)> ThreadPoolFn;		// param is worker index

		void Start(const char* poolName, uint32_t threadCount, ThreadPoolFn threadfn, ThreadPoolFn threadStartFn = nullptr, int threadStartIndex=0);
		void Stop();

	private:
		class PooledThread;
		std::vector<std::unique_ptr<PooledThread>> m_threads;
		std::atomic<int32_t> m_stopRequested;
		ThreadPoolFn m_fn;
		ThreadPoolFn m_initFn;
		int m_threadStartIndex = 0;		// for multiple thread pools using the same initfn
	};
}