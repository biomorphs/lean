#include "frame_graph.h"
#include "time_system.h"
#include "job_system.h"

namespace Engine
{
	bool FrameGraph::FixedUpdateSequenceNode::Run() {
		char debugName[1024] = { '\0' };
		sprintf_s(debugName, "%s", m_displayName.c_str());
		SDE_PROF_EVENT_DYN(debugName);
		auto timeSys = Engine::GetSystem<TimeSystem>("Time");
		double startTime = timeSys->GetElapsedTimeReal();
		const double c_maxUpdateTime = timeSys->GetFixedUpdateDelta() * 1.618;
		while (timeSys->GetFixedUpdateCatchupTime() >= timeSys->GetFixedUpdateDelta() &&
			(timeSys->GetElapsedTimeReal() - startTime) < c_maxUpdateTime)
		{
			SDE_PROF_EVENT("FixedUpdate");
			bool result = true;
			for (auto& it : m_children)
			{
				if (!it->Run())
				{
					return false;
				}
			}
			timeSys->FixedUpdateEnd();
		}
		return true;
	}

	bool FrameGraph::SequenceNode::Run() {
		char debugName[1024] = { '\0' };
		sprintf_s(debugName, "%s", m_displayName.c_str());
		SDE_PROF_EVENT_DYN(debugName);
		bool result = true;
		for (auto& it : m_children)
		{
			result &= it->Run();
			if (!result)
			{
				break;
			}
		}
		return result;
	}

	bool FrameGraph::AsyncNode::Run() {
		char debugName[1024] = { '\0' };
		sprintf_s(debugName, "%s", m_displayName.c_str());
		SDE_PROF_EVENT_DYN(debugName);
		static auto jobs = GetSystem<JobSystem>("Jobs");

		// kick off jobs asap
		struct JobDesc {
			bool m_ran = false;
			bool m_result = false;
		};
		std::vector<JobDesc> jobDescs;
		std::atomic<int> jobRunCount = std::max((int)m_children.size() - 1, 0);
		if (m_children.size() > 1)
		{
			jobDescs.resize(m_children.size() - 1);
			for (int j = 1; j < m_children.size(); ++j)
			{
				jobs->PushSlowJob([&jobDescs, j, &jobRunCount, this](void*) {
					SDE_PROF_EVENT();
					jobDescs[j - 1].m_result = m_children[j]->Run();
					jobDescs[j - 1].m_ran = true;
					jobRunCount--;
				});
			}
		}
		// run first job on current thread but after jobs were submitted
		// (cheaper than kicking out a job)
		bool result = true;
		if (m_children.size() > 0)
		{
			if (!m_children[0]->Run())
			{
				result = false;
			}
		}

		// wait for the results
		while (jobRunCount > 0)
		{
			jobs->ProcessJobThisThread();
		}

		for (int i = 0; i < jobDescs.size() && result == true; ++i)
		{
			result &= (jobDescs[i].m_ran == true && jobDescs[i].m_result == true);
		}
		return result;
	}

	FrameGraph::Node* FrameGraph::Node::FindInternal(Node* parent, std::string_view name)
	{
		if (parent->m_displayName == name)
		{
			return parent;
		}
		for (auto& it : parent->m_children)
		{
			Node* foundInChild = FindInternal(it.get(), name);
			if (foundInChild)
			{
				return foundInChild;
			}
		}
		return nullptr;
	}

	FrameGraph::Node* FrameGraph::Node::FindFirst(std::string name)
	{
		return FindInternal(this, name);
	}

	FrameGraph::FixedUpdateSequenceNode* FrameGraph::Node::AddFixedUpdateInternal(std::string name)
	{
		SDE_PROF_EVENT();
		auto newSeq = std::make_unique<FixedUpdateSequenceNode>();
		auto returnPtr = newSeq.get();
		newSeq->m_displayName = "FixedUpdateSequence - " + name;
		m_children.push_back(std::move(newSeq));
		return returnPtr;
	}

	FrameGraph::SequenceNode* FrameGraph::Node::AddSequenceInternal(std::string name)
	{
		SDE_PROF_EVENT();
		auto newSeq = std::make_unique<SequenceNode>();
		auto returnPtr = newSeq.get();
		newSeq->m_displayName = "Sequence - " + name;
		m_children.push_back(std::move(newSeq));
		return returnPtr;
	}

	FrameGraph::AsyncNode* FrameGraph::Node::AddAsyncInternal(std::string name)
	{
		SDE_PROF_EVENT();
		auto newAsync = std::make_unique<AsyncNode>();
		auto returnPtr = newAsync.get();
		newAsync->m_displayName = "Async - " + name;
		m_children.push_back(std::move(newAsync));
		return returnPtr;
	}

	FrameGraph::FnNode* FrameGraph::Node::AddFnInternal(std::string name)
	{
		SDE_PROF_EVENT();
		auto newFn = std::make_unique<FnNode>();
		auto returnPtr = newFn.get();
		newFn->m_displayName = "Fn - " + name;
		newFn->m_fn = SystemManager::GetInstance().GetTickFn(name);
		m_children.push_back(std::move(newFn));
		return returnPtr;
	}
}