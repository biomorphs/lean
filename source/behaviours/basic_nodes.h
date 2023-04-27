#pragma once
#include "node.h"

namespace Behaviours
{
	class BehaviourTree;
	class RootNode : public Node
	{
	public:
		SERIALISED_CLASS();
		RootNode() 
		{	
			m_name = "Root";
			m_bgColour = { 0.8f,0.0f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 60, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	class SequenceNode : public Node
	{
	public:
		SERIALISED_CLASS();
		SequenceNode()
		{
			m_name = "Sequence";
			m_bgColour = { 0.5f,0.5f,0.5f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 100, 32 };
		}
		void ShowEditorGui(Engine::DebugGuiSystem&);
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	class SelectorNode : public Node
	{
	public:
		SERIALISED_CLASS();
		SelectorNode()
		{
			m_name = "Selector";
			m_bgColour = { 0.3f,0.4f,0.8f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 100, 32 };
		}
		void ShowEditorGui(Engine::DebugGuiSystem&);
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	class RepeaterNode : public Node
	{
	public:
		SERIALISED_CLASS();
		RepeaterNode()
		{
			m_name = "Repeater";
			m_bgColour = { 0.7f,0.7f,0.1f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 90, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		std::unique_ptr<RunningNodeContext> PrepareContext() const;
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		RunningState Tick(RunningNodeContext*, BehaviourTreeInstance&) const;
		void ShowEditorGui(Engine::DebugGuiSystem&);
		int m_repeatCount = 1;
	};

	// Has 1 child, always returns success (or running)
	class SucceederNode : public Node
	{
	public:
		SERIALISED_CLASS();
		SucceederNode()
		{
			m_name = "Succeeder";
			m_bgColour = { 0.0f,0.6f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 80, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};
}