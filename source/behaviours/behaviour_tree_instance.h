#pragma once
#include "node.h"
#include "blackboard.h"
#include <stack>

namespace Behaviours
{
	class BehaviourTree;
	class BehaviourTreeInstance
	{
	public:
		explicit BehaviourTreeInstance(const BehaviourTree* bt);
		~BehaviourTreeInstance() = default;

		void Reset();
		void Tick();
		RunningState TickNode(Node* n);

		struct NodeContextData {
			RunningState m_runningState = RunningState::NotRan;
			std::unique_ptr<RunningNodeContext> m_execContext;
			int m_initCount = 0;
			int m_tickCount = 0;
		};
		RunningState ExecuteTick(Node* n);
		NodeContextData* FindContextData(Node* n);
		Blackboard m_bb;
		const BehaviourTree* m_tree;
		std::vector<NodeContextData> m_nodeContexts;
	};
}