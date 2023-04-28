#pragma once
#include "behaviour_tree.h"
#include "engine/system.h"
#include "engine/debug_gui_menubar.h"
#include <vector>
#include <string>
#include <functional>

namespace Behaviours
{
	class BehaviourTree;
	class Node;
	class BehaviourTreeEditor : public Engine::System
	{
	public:
		using OnNodesChangedFn = std::function<void()>;
		BehaviourTreeEditor();
		virtual ~BehaviourTreeEditor();
		virtual bool Tick(float timeDelta);
	private:
		void LoadTree(std::string_view path);
		void SaveTree(std::string_view path);
		void ShowTreeEditor();
		void ShowNodeEditor();
		void DoOnClose();
		bool m_windowOpen = false;
		Engine::MenuBar m_menuBar;
		uint32_t m_currentEditingNode = -1;
		std::unique_ptr<BehaviourTree> m_currentEditingTree;
		std::string m_currentTreePath = "";
	};
}