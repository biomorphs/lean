#include "job.h"
#include <cassert>

namespace Engine
{
	Job::Job(JobSystem* parent, JobThreadFunction threadFn)
		: m_parent(parent)
		, m_threadFn(threadFn)
	{
		assert(parent != nullptr);
	}

	void Job::Run()
	{
		m_threadFn();
	}
}