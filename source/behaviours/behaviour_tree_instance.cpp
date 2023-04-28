#include "behaviour_tree_instance.h"
#include "behaviour_tree.h"
#include "core/profiler.h"

namespace Behaviours
{
	BehaviourTreeInstance::BehaviourTreeInstance(const BehaviourTree* bt)
		: m_tree(bt)
	{
		SDE_PROF_EVENT();
		m_nodeContexts.resize(bt->m_allNodes.size());
	}

	void BehaviourTreeInstance::Reset(bool clearBlackboard)
	{
		SDE_PROF_EVENT();
		m_nodeContexts.resize(m_tree->m_allNodes.size());
		for (int i = 0; i < m_nodeContexts.size(); i++)
		{
			m_nodeContexts[i].m_runningState = RunningState::NotRan;
			m_nodeContexts[i].m_execContext = m_tree->m_allNodes[i]->PrepareContext();
			m_nodeContexts[i].m_initCount = 0;
			m_nodeContexts[i].m_tickCount = 0;
		}
		if (clearBlackboard)
		{
			m_bb.Reset();
		}
	}

	BehaviourTreeInstance::NodeContextData* BehaviourTreeInstance::FindContextData(Node* node)
	{
		SDE_PROF_EVENT();
		auto foundNode = std::find_if(m_tree->m_allNodes.begin(), m_tree->m_allNodes.end(), [node](const std::unique_ptr<Node>& n) {
			return n.get() == node;
		});
		if (foundNode != m_tree->m_allNodes.end())
		{
			int index = std::distance(m_tree->m_allNodes.begin(), foundNode);
			return &m_nodeContexts[index];
		}
		else
		{
			return nullptr;
		}
	}

	RunningState BehaviourTreeInstance::ExecuteTick(Node* n)
	{
		SDE_PROF_EVENT();
		NodeContextData* ncd = FindContextData(n);
		if (ncd->m_runningState == RunningState::NotRan)
		{
			n->Init(ncd->m_execContext.get(), *this);
			ncd->m_initCount++;
			ncd->m_runningState = RunningState::Running;
		}
		if (ncd->m_runningState == RunningState::Running)
		{
			ncd->m_runningState = n->Tick(ncd->m_execContext.get(), *this);
			ncd->m_tickCount++;
		}
		return ncd->m_runningState;
	}

	void BehaviourTreeInstance::Tick(bool restartOnSuccessOrFail)
	{
		SDE_PROF_EVENT();
		Node* rootNodePtr = m_tree->GetRoot();
		if (rootNodePtr != nullptr)
		{
			if (restartOnSuccessOrFail)
			{
				NodeContextData* rootContext = FindContextData(rootNodePtr);
				bool failedOrSucceeded = (rootContext->m_runningState == RunningState::Failed || rootContext->m_runningState == RunningState::Success);
				if (failedOrSucceeded && rootContext->m_initCount > 0 && rootContext->m_tickCount > 0)
				{
					Reset(false);	// reset node state but not the blackboard
				}
			}				
			ExecuteTick(rootNodePtr);
		}
	}
}