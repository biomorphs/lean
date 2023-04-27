#include "behaviour_tree_editor.h"
#include "behaviour_tree_system.h"
#include "behaviour_tree.h"
#include "node_editor.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"

namespace Behaviours
{
	BehaviourTreeEditor::BehaviourTreeEditor()
	{
	}

	bool BehaviourTreeEditor::ShowEditor(BehaviourTree& tree, const std::vector<std::string>& nodePrototypeNames, OnNodesChangedFn onNodesChanged)
	{
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
		bool windowOpen = true;
		if (dbgGui->BeginWindow(windowOpen, "Behaviour Tree Editor"))
		{
			for (int n = 0; n < tree.m_allNodes.size(); ++n)
			{
				std::string editLbl = "Node #" + std::to_string(tree.m_allNodes[n]->m_localID) + " - '" + tree.m_allNodes[n]->m_name + "'##" + std::to_string(n);
				if (dbgGui->Button(editLbl.c_str()))
				{
					m_currentEditingNode = n;
				}
			}
			dbgGui->Separator();
			int currentVal = 0;
			if (dbgGui->ComboBox("Add Node", nodePrototypeNames, currentVal))
			{
				behSys->AddNode(tree, nodePrototypeNames[currentVal]);
				onNodesChanged();
			}
			dbgGui->EndWindow();

			if (m_currentEditingNode != -1 && m_currentEditingNode < tree.m_allNodes.size())
			{
				NodeEditor ne;
				if (!ne.ShowEditor(tree.m_allNodes[m_currentEditingNode].get()))
				{
					m_currentEditingNode = -1;
				}
			}
		}

		return true;
	}
}
