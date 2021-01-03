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

		void PushJob(Job::JobThreadFunction threadFn);
		void SetThreadInitFn(std::function<void(uint32_t)> fn);

	private:
		class RenderSystem* m_renderSystem;
		Core::ThreadPool m_threadPool;
		JobQueue m_pendingJobs;
		Kernel::Semaphore m_jobThreadTrigger;
		std::atomic<int32_t> m_jobThreadStopRequested;
		int32_t m_threadCount;
		std::function<void(uint32_t)> m_threadInitFn = nullptr;
	};
}