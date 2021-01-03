#pragma once

#include <functional>
#include <string>

namespace Engine
{
	class JobSystem;
	class Job
	{
	public:
		typedef std::function<void()> JobThreadFunction;	// Code to be ran on the job thread
		Job() = default;
		Job(JobSystem* parent, JobThreadFunction threadFn);
		~Job() = default;
		Job(Job&&) = default;
		Job& operator=(Job&&) = default;
		Job(const Job&) = delete;
		Job& operator=(const Job&) = delete;
		
		void Run();

	private:
		JobThreadFunction m_threadFn = nullptr;
		JobSystem* m_parent = nullptr;
		uint64_t m_padding[8] = { 0 };
	};
}