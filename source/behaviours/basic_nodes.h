#pragma once
#include "node.h"

namespace Behaviours
{
	class BehaviourTree;

	// Tree root
	class RootNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		RootNode() 
		{	
			m_bgColour = { 0.8f,0.0f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 60, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		virtual std::string_view GetTypeName() const { return "Root"; }
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	// returns success if all children are succesful
	class SequenceNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		SequenceNode()
		{
			m_bgColour = { 0.5f,0.5f,0.5f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 100, 32 };
		}
		virtual std::string_view GetTypeName() const { return "Sequence"; }
		void ShowEditorGui(Engine::DebugGuiSystem&);
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	// returns success if any child is succesful
	class SelectorNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		SelectorNode()
		{
			m_bgColour = { 0.3f,0.4f,0.8f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 100, 32 };
		}
		virtual std::string_view GetTypeName() const { return "Selector"; }
		void ShowEditorGui(Engine::DebugGuiSystem&);
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	// calls the child repeatedly until repeat count is hit or child fails
	class RepeaterNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		RepeaterNode()
		{
			m_bgColour = { 0.7f,0.7f,0.1f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 90, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		virtual std::string_view GetTypeName() const { return "Repeater"; }
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
		virtual SERIALISED_CLASS();
		SucceederNode()
		{
			m_bgColour = { 0.0f,0.6f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 80, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		virtual std::string_view GetTypeName() const { return "Succeeder"; }
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	// Has 1 child, always returns opposite of child result (or running)
	class InverterNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		InverterNode()
		{
			m_bgColour = { 0.8f,0.0f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 80, 32 };
			m_outputPins.push_back({
				{1,1,1}, ""
			});
		}
		virtual std::string_view GetTypeName() const { return "Inverter"; }
		void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const;
	};

	class CompareFloatsNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		CompareFloatsNode()
		{
			m_bgColour = { 0.0f,0.6f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 120, 32 };
		}
		virtual std::string_view GetTypeName() const { return "CompareFloats"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance&) const;
		void ShowEditorGui(Engine::DebugGuiSystem&);

		enum class ComparisonType : int
		{
			LessThan,
			EqualTo,
			GreaterThan,
			LessThanEqual,
			GreaterThanEqual
		};
		std::string m_value0;
		std::string m_value1;
		ComparisonType m_comparison = ComparisonType::LessThan;
	};

	class RunTreeNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		RunTreeNode()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 1,1,1 };
			m_editorDimensions = { 120, 32 };
		}
		virtual std::string_view GetTypeName() const { return "RunBehaviourTree"; }
		virtual std::unique_ptr<RunningNodeContext> PrepareContext() const;
		virtual void Init(RunningNodeContext*, BehaviourTreeInstance&) const;
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance&) const;
		void ShowEditorGui(Engine::DebugGuiSystem&);

		std::string m_treePath;
	};
}