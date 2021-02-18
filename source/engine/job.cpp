#include "job.h"
#include "core/profiler.h"
#include <cassert>

namespace Engine
{
	Job::Job(JobSystem* parent, JobThreadFunction threadFn, void* userData)
		: m_parent(parent)
		, m_threadFn(threadFn)
		, m_userData(userData)
	{
		assert(parent != nullptr);
	}

	void Job::Run()
	{
		SDE_PROF_EVENT();
		m_threadFn(m_userData);
	}
}