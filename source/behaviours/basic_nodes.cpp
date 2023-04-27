#include "basic_nodes.h"
#include "behaviour_tree.h"
#include "behaviour_tree_instance.h"
#include "engine/debug_gui_system.h"

namespace Behaviours
{
	void InitChildren(const Node* n, BehaviourTreeInstance& bti)
	{
		for (int p = 0; p < n->m_outputPins.size(); ++p)
		{
			Node* child = bti.m_tree->FindNode(n->m_outputPins[p].m_connectedNodeID);
			if (child)
			{
				auto ncd = bti.FindContextData(child);
				ncd->m_runningState = RunningState::NotRan;
			}
		}
	}

	SERIALISE_BEGIN_WITH_PARENT(RootNode, Node)
	SERIALISE_END();

	void RootNode::Init(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		InitChildren(this, bti);
	}

	RunningState RootNode::Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		Node* child = bti.m_tree->FindNode(m_outputPins[0].m_connectedNodeID);
		if (child)
		{
			return bti.ExecuteTick(child);
		}
		return RunningState::Success;
	}

	SERIALISE_BEGIN_WITH_PARENT(SequenceNode, Node)
	SERIALISE_END();

	void SequenceNode::Init(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		InitChildren(this, bti);
	}

	RunningState SequenceNode::Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		for (int p = 0; p < m_outputPins.size(); ++p)
		{
			Node* child = bti.m_tree->FindNode(m_outputPins[p].m_connectedNodeID);
			if (child)
			{
				auto result = bti.ExecuteTick(child);
				if (result == RunningState::Running || result == RunningState::Failed)
				{
					return result;
				}
			}
		}
		return RunningState::Success;
	}

	void SequenceNode::ShowEditorGui(Engine::DebugGuiSystem& dbgGui)
	{
		if (dbgGui.Button("Add Output"))
		{
			m_outputPins.push_back({
				{0.5f,0.5f,0.5f}, ""
			});
		}
	}

	SERIALISE_BEGIN_WITH_PARENT(SelectorNode, Node)
	SERIALISE_END();

	void SelectorNode::Init(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		InitChildren(this, bti);
	}

	RunningState SelectorNode::Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		for (int p = 0; p < m_outputPins.size(); ++p)
		{
			Node* child = bti.m_tree->FindNode(m_outputPins[p].m_connectedNodeID);
			if (child)
			{
				auto result = bti.ExecuteTick(child);
				if (result == RunningState::Running || result == RunningState::Success)
				{
					return result;
				}
			}
		}
		return RunningState::Failed;	// if we hit his point no children succeeded
	}

	void SelectorNode::ShowEditorGui(Engine::DebugGuiSystem& dbgGui)
	{
		if (dbgGui.Button("Add Output"))
		{
			m_outputPins.push_back({
				{0.5f,0.5f,0.5f}, ""
			});
		}
	}

	SERIALISE_BEGIN_WITH_PARENT(RepeaterNode, Node)
		SERIALISE_PROPERTY("RepeatCount", m_repeatCount)
	SERIALISE_END();

	struct RepeaterContext : public RunningNodeContext
	{
		int m_count = 0;
	};
	std::unique_ptr<RunningNodeContext> RepeaterNode::PrepareContext() const
	{
		return std::make_unique<RepeaterContext>();
	}

	void RepeaterNode::Init(RunningNodeContext* ctx, BehaviourTreeInstance& bti) const
	{
		RepeaterContext* rptCtx = static_cast<RepeaterContext*>(ctx);
		InitChildren(this, bti);
		rptCtx->m_count = 0;
	}

	RunningState RepeaterNode::Tick(RunningNodeContext* ctx, BehaviourTreeInstance& bti) const
	{
		RepeaterContext* rptCtx = static_cast<RepeaterContext*>(ctx);
		if (rptCtx->m_count >= m_repeatCount)
		{
			return RunningState::Success;
		}

		Node* child = bti.m_tree->FindNode(m_outputPins[0].m_connectedNodeID);
		if (child)
		{
			for (int i = rptCtx->m_count; i < m_repeatCount; ++i)
			{
				auto result = bti.ExecuteTick(child);
				if (result == RunningState::Running || result == RunningState::Failed)
				{
					return result;
				}
				else if (result == RunningState::Success)
				{
					rptCtx->m_count++;
					if (rptCtx->m_count < m_repeatCount)
					{
						InitChildren(this, bti);	// reset children to not ran for next iteration
					}
				}
			}
		}
		
		return RunningState::Success;
	}

	void RepeaterNode::ShowEditorGui(Engine::DebugGuiSystem& dbgGui)
	{
		m_repeatCount = dbgGui.InputInt("Repeat Count", m_repeatCount, 1, 1);
	}

	SERIALISE_BEGIN_WITH_PARENT(SucceederNode, Node)
	SERIALISE_END();

	void SucceederNode::Init(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		InitChildren(this, bti);
	}

	RunningState SucceederNode::Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
	{
		Node* child = bti.m_tree->FindNode(m_outputPins[0].m_connectedNodeID);
		if (child)
		{
			auto result = bti.ExecuteTick(child);
			if (result == RunningState::Running)
			{
				return result;
			}
			else
			{
				return RunningState::Success;
			}
		}
		return RunningState::Success;
	}
}