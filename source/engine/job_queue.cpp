#include "job_queue.h"

namespace Engine
{
	JobQueue::JobQueue()
	{
	}

	JobQueue::~JobQueue()
	{
	}

	void JobQueue::PushJob(Job&& j)
	{
		std::lock_guard<std::mutex> guard(m_lock);
		m_currentJobs.push(std::move(j));
	}

	bool JobQueue::PopJob(Job &j)
	{
		std::lock_guard<std::mutex> guard(m_lock);
		if (!m_currentJobs.empty())
		{
			j = std::move(m_currentJobs.front());
			m_currentJobs.pop();
			return true;
		}

		return false;
	}

	void JobQueue::RemoveAll()
	{
		std::lock_guard<std::mutex> guard(m_lock);
		while (!m_currentJobs.empty())
		{
			m_currentJobs.pop();
		}
	}
}