#pragma once
#include "job.h"
#include "core/mutex.h"
#include <queue>

namespace Engine
{
	class JobQueue
	{
	public:
		JobQueue();
		~JobQueue();

		void PushJob(Job&& j);
		bool PopJob(Job &j);
		void RemoveAll();

	private:
		Core::Mutex m_mutex;
		std::queue<Job> m_currentJobs;
	};
}