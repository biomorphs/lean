#pragma once
#include "job.h"
#include <queue>
#include <mutex>

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
		std::mutex m_lock;
		std::queue<Job> m_currentJobs;
	};
}