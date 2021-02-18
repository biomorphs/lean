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
		Core::ScopedMutex lock(m_mutex);
		m_currentJobs.push(std::move(j));
	}

	bool JobQueue::PopJob(Job &j)
	{
		Core::ScopedMutex lock(m_mutex);
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
		Core::ScopedMutex lock(m_mutex);
		while (!m_currentJobs.empty())
		{
			m_currentJobs.pop();
		}
	}
}