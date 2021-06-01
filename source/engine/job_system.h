#pragma once

#include "job_queue.h"
#include "system.h"
#include "core/thread_pool.h"
#include "core/semaphore.h"
#include <atomic>

namespace Engine
{
	class JobSystem : public System
	{
	public:
		JobSystem();
		virtual ~JobSystem();

		bool PostInit();
		void Shutdown();

		void ForEachAsync(int start, int end, int step, int stepsPerJob, std::function<void(int32_t)> fn);
		void ProcessJobThisThread();
		void PushSlowJob(Job::JobThreadFunction threadFn);
		void PushJob(Job::JobThreadFunction threadFn);
		void SetThreadInitFn(std::function<void(uint32_t)> fn);
		int32_t inline GetThreadCount() const { return m_threadCount; }

	private:
		Core::ThreadPool m_threadPool;
		Core::ThreadPool m_threadPoolSlow;
		JobQueue m_pendingJobs;
		JobQueue m_pendingSlowJobs;
		Core::Semaphore m_jobThreadTrigger;
		Core::Semaphore m_slowJobThreadTrigger;
		std::atomic<int32_t> m_jobThreadStopRequested;
		int32_t m_threadCount;
		std::function<void(uint32_t)> m_threadInitFn = nullptr;
	};
}