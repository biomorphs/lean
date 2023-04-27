 #pragma once
#include "core/glm_headers.h"
#include "engine/serialisation.h"
#include <string>
#include <vector>

namespace Engine
{
	class DebugGuiSystem;
}

namespace Behaviours
{
	enum class RunningState : int
	{
		NotRan,
		Success,
		Failed,
		Running
	};

	struct PinProperties
	{
		SERIALISED_CLASS();
		glm::vec3 m_colour = glm::vec3(0.0f,0.0f,0.0f);
		std::string m_annotation;
		uint16_t m_connectedNodeID = (uint16_t)-1;
	};

	// used as a base to allow nodes to provide custom execution context
	class RunningNodeContext {
	};
	class BehaviourTreeInstance;
	class Blackboard;
	class Node;
	using ExecuteNodeFn = std::function<RunningState(Node*)>;
	class Node
	{
	public:
		Node() = default;
		virtual ~Node() {}
		virtual void ShowEditorGui(Engine::DebugGuiSystem&) { }

		virtual std::unique_ptr<RunningNodeContext> PrepareContext() const { return nullptr; }	// called once when tree is reset
		virtual void Init(RunningNodeContext*, BehaviourTreeInstance&) const { }	// called when a parent executes a child for the first time. State will be set to running
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance&) const { return RunningState::Failed; }

		SERIALISED_CLASS();
		std::vector<PinProperties> m_outputPins;
		uint16_t m_localID = -1;	// id is local to the behaviour tree containing this node
		std::string m_name = "Node";
		glm::vec3 m_bgColour = glm::vec3(0.8f,0.8f,0.8f);
		glm::vec3 m_textColour = glm::vec3(0.0f,0.0f,0.0f);
		glm::vec2 m_editorPosition = glm::vec2(0.0f, 0.0f);
		glm::vec2 m_editorDimensions = glm::vec2(64.0f, 32.0f);
	};
}
