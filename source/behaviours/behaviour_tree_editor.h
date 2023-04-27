#pragma once
#include "engine/debug_gui_menubar.h"
#include <vector>
#include <string>
#include <functional>

namespace Behaviours
{
	class BehaviourTree;
	class Node;
	class BehaviourTreeEditor
	{
	public:
		using OnNodesChangedFn = std::function<void()>;
		BehaviourTreeEditor();
		bool ShowEditor(BehaviourTree& tree, const std::vector<std::string>& nodePrototypeNames, OnNodesChangedFn onNodesChanged);
	private:
		uint32_t m_currentEditingNode = -1;
		Engine::MenuBar m_menuBar;
	};
}