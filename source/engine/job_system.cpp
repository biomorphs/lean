#include "job_system.h"
#include "core/profiler.h"
#include "SDL_cpuinfo.h"
#include <cassert>

namespace Engine
{
	JobSystem::JobSystem()
		: m_threadCount(8)
		, m_jobThreadTrigger(0)
		, m_slowJobThreadTrigger(0)
		, m_jobThreadStopRequested(0)
	{
		int cpuCount = SDL_GetCPUCount();
		if (cpuCount > 2)
		{
			m_threadCount = cpuCount - 1;
		}
	}

	JobSystem::~JobSystem()
	{
	}

	void JobSystem::SetThreadInitFn(std::function<void(uint32_t)> fn)
	{
		assert(m_threadInitFn == nullptr);
		m_threadInitFn = fn;
	}

	bool JobSystem::PostInit()
	{
		SDE_PROF_EVENT();
		int32_t threadsPerPool = std::max(1, m_threadCount);
		auto jobThread = [this](uint32_t threadIndex)
		{
			if (m_jobThreadStopRequested == 0)	// This is to stop deadlock on the semaphore when shutting down
			{
				{
					SDE_PROF_STALL("WaitForJobs");
					m_jobThreadTrigger.Wait();		// Wait for jobs
				}
				Job currentJob;
				if (m_pendingJobs.PopJob(currentJob))	// 'fast' jobs always have priority
				{
					currentJob.Run();
				}
				else
				{
					// If no job was handled (due to scheduling), re-trigger threads
					m_jobThreadTrigger.Post();
				}
			}
		};
		m_threadPool.Start("JobSystem", threadsPerPool, jobThread);
		auto jobThreadSlow = [this](uint32_t threadIndex)
		{
			if (m_jobThreadStopRequested == 0)	// This is to stop deadlock on the semaphore when shutting down
			{
				{
					SDE_PROF_STALL("WaitForJobs");
					m_slowJobThreadTrigger.Wait();
				}
				Job currentJob;
				if (m_pendingSlowJobs.PopJob(currentJob))
				{
					currentJob.Run();
				}
				else
				{
					// If no job was handled (due to scheduling), re-trigger threads
					m_slowJobThreadTrigger.Post();
				}
			}
		};
		m_threadPoolSlow.Start("JobSystem_Slow", threadsPerPool, jobThreadSlow, m_threadInitFn);
		return true;
	}

	void JobSystem::Shutdown()
	{
		SDE_PROF_EVENT();

		// Clear out pending jobs, we do not flush under any circumstances!
		m_pendingJobs.RemoveAll();
		m_pendingSlowJobs.RemoveAll();

		// At this point, jobs may still be running, or the threads may be waiting
		// on the trigger. In order to ensure the jobs finish, we set the quitting flag, 
		// then post the semaphore for each thread, and stop the thread pool
		// This should ensure we don't deadlock on shutdown
		m_jobThreadStopRequested = 1;

		for (int32_t t = 0; t < m_threadCount; ++t)
		{
			m_jobThreadTrigger.Post();
			m_slowJobThreadTrigger.Post();
		}

		// Stop the threadpool, no more jobs will be taken after this
		m_threadPool.Stop();
		m_threadPoolSlow.Stop();
	}

	void JobSystem::PushSlowJob(Job::JobThreadFunction threadFn)
	{
		SDE_PROF_EVENT();

		Job jobDesc(this, threadFn);
		m_pendingSlowJobs.PushJob(std::move(jobDesc));
		m_slowJobThreadTrigger.Post();		// Trigger threads
	}

	void JobSystem::PushJob(Job::JobThreadFunction threadFn)
	{
		SDE_PROF_EVENT();

		Job jobDesc(this, threadFn);
		m_pendingJobs.PushJob(std::move(jobDesc));
		m_jobThreadTrigger.Post();		// Trigger threads
	}
}