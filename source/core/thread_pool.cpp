#include "thread_pool.h"
#include "thread.h"
#include "profiler.h"
#include <cassert>

namespace Core
{
	class ThreadPool::PooledThread
	{
	public:
		PooledThread(const std::string& name, ThreadPool* parent, uint32_t workerIndex)
			: m_name(name)
			, m_parent(parent)
			, m_workerIndex(workerIndex)
			, m_hasInitialised(false)
		{
		}
		~PooledThread()
		{
		}
		void Start()
		{
			assert(m_parent != nullptr);
			auto threadFnc = [this]()
			{
				SDE_PROF_THREAD(m_name.c_str());
				if (m_parent->m_initFn != nullptr)
				{
					m_parent->m_initFn(m_parent->m_threadStartIndex + m_workerIndex);
				}
				m_hasInitialised = true;
				while (m_parent->m_stopRequested == 0)
				{
					m_parent->m_fn(m_parent->m_threadStartIndex + m_workerIndex);
				}
				return 0;
			};
			m_thread.Create(m_name.c_str(), threadFnc);
		}
		void WaitForFinish()
		{
			SDE_PROF_STALL("PooledThread::WaitForFinish");
			m_thread.WaitForFinish();
		}
		bool HasInitialised()
		{
			return m_hasInitialised;
		}
	private:
		Thread m_thread;
		ThreadPool* m_parent;
		std::atomic<bool> m_hasInitialised;
		std::string m_name;
		uint32_t m_workerIndex;
	};

	ThreadPool::ThreadPool()
		: m_stopRequested(0)
		, m_threadStartIndex(0)
	{
	}

	ThreadPool::~ThreadPool()
	{
		Stop();
	}

	void ThreadPool::Start(const char* poolName, uint32_t threadCount, ThreadPoolFn threadfn, ThreadPoolFn threadStartFn, int threadStartIndex)
	{
		SDE_PROF_EVENT();
		assert(m_threads.size() == 0);
		m_fn = threadfn;
		m_initFn = threadStartFn;
		m_threadStartIndex = threadStartIndex;
		m_threads.reserve(threadCount);
		for (uint32_t t = 0;t < threadCount;++t)
		{
			char threadNameBuffer[256] = { '\0' };
			sprintf_s(threadNameBuffer, "%s_%d", poolName, t);
			auto thisThread = std::make_unique<PooledThread>(threadNameBuffer, this, t);
			assert(thisThread);
			thisThread->Start();
			while (!thisThread->HasInitialised())
			{
				SDE_PROF_STALL("Wait for thread init");
				Core::Thread::Sleep(0);
			}
			m_threads.push_back(std::move(thisThread));
		}
	}

	void ThreadPool::Stop()
	{
		SDE_PROF_EVENT();
		m_stopRequested = true;
		for (auto& it : m_threads)
		{
			it->WaitForFinish();
			it = nullptr;
		}
		m_threads.clear();
	}
}