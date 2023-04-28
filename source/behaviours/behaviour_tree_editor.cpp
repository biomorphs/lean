#include "behaviour_tree_editor.h"
#include "behaviour_tree_system.h"
#include "behaviour_tree_renderer.h"
#include "node_editor.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"
#include "engine/file_picker_dialog.h"
#include "core/file_io.h"

namespace Behaviours
{
	BehaviourTreeEditor::BehaviourTreeEditor()
	{
		SDE_PROF_EVENT();
		auto& fileMenu = m_menuBar.AddSubmenu("File");
		fileMenu.AddItem("New Tree", [this]() {
			m_currentEditingTree = std::make_unique<BehaviourTree>();
			m_currentEditingNode = -1;
			m_currentTreePath = "";
		});
		fileMenu.AddItem("Load Tree", [this]() {
			std::string targetPath = Engine::ShowFilePicker("Load Tree", "", "Tree Files (.beht)\0*.beht\0");
			LoadTree(targetPath);
		});
		fileMenu.AddItem("Save", [this]() {
			if (m_currentEditingTree != nullptr)
			{
				std::string targetPath = m_currentTreePath;
				if (targetPath == "")
				{
					targetPath = Engine::ShowFilePicker("Save Tree", "", "Tree Files (.beht)\0*.beht\0", true);
				}
				SaveTree(targetPath);
			}
		});
		fileMenu.AddItem("Close", [this]() {
			DoOnClose();
		});
	}

	BehaviourTreeEditor::~BehaviourTreeEditor()
	{
	}

	void BehaviourTreeEditor::LoadTree(std::string_view path)
	{
		SDE_PROF_EVENT();
		std::string fileText;
		if (!Core::LoadTextFromFile(path.data(), fileText))
		{
			return;
		}

		nlohmann::json treeJson;
		{
			SDE_PROF_EVENT("ParseJson");
			treeJson = nlohmann::json::parse(fileText);
		}

		m_currentEditingTree = std::make_unique<BehaviourTree>();
		m_currentEditingTree->Serialise(treeJson, Engine::SerialiseType::Read);
		m_currentEditingNode = -1;
		m_currentTreePath = path;
	}

	void BehaviourTreeEditor::SaveTree(std::string_view path)
	{
		SDE_PROF_EVENT();
		if (m_currentEditingTree != nullptr && path.size() > 0)
		{
			nlohmann::json treeJson;
			m_currentEditingTree->Serialise(treeJson, Engine::SerialiseType::Write);
			bool result = Core::SaveTextToFile(path.data(), treeJson.dump(2));
			if (result)
			{
				m_currentTreePath = path;
				auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
				behSys->OnTreeModified(path);
			}
		}
	}

	void BehaviourTreeEditor::DoOnClose()
	{
		m_windowOpen = false;
	}

	bool BehaviourTreeEditor::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");

		Engine::MenuBar mainMenu;
		auto& particlesMenu = mainMenu.AddSubmenu(ICON_FK_LEAF " Behaviours");
		particlesMenu.AddItem("Open Tree Editor", [this]() {
			m_windowOpen = true;
		});
		dbgGui->MainMenuBar(mainMenu);

		if (m_windowOpen)
		{
			ShowTreeEditor();
			ShowNodeEditor();
			if (m_currentEditingTree != nullptr)
			{
				BehaviourTreeRenderer btr;
				btr.DrawTree(*m_currentEditingTree, nullptr);
			}
		}

		return true;
	}

	void BehaviourTreeEditor::ShowNodeEditor()
	{
		SDE_PROF_EVENT();
		if (m_currentEditingTree != nullptr && m_currentEditingNode != -1 && m_currentEditingNode < m_currentEditingTree->m_allNodes.size())
		{
			NodeEditor ne;
			if (!ne.ShowEditor(m_currentEditingTree->m_allNodes[m_currentEditingNode].get()))
			{
				m_currentEditingNode = -1;
			}
		}
	}

	void BehaviourTreeEditor::ShowTreeEditor()
	{
		SDE_PROF_EVENT();
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
		dbgGui->BeginWindow(m_windowOpen, "Behaviour Tree Editor", Engine::GuiWindowFlags::HasMenuBar);
		dbgGui->WindowMenuBar(m_menuBar);
		if (m_currentEditingTree != nullptr)
		{
			for (int n = 0; n < m_currentEditingTree->m_allNodes.size(); ++n)
			{
				std::string nodeType(m_currentEditingTree->m_allNodes[n]->GetTypeName());
				std::string editLbl = nodeType + " #" + std::to_string(m_currentEditingTree->m_allNodes[n]->m_localID) + " - '" + m_currentEditingTree->m_allNodes[n]->m_name + "'##" + std::to_string(n);
				if (dbgGui->Button(editLbl.c_str()))
				{
					m_currentEditingNode = n;
				}
			}
			dbgGui->Separator();
			int currentVal = 0;
			const auto& typeNames = behSys->GetNodeTypeNames();
			if (dbgGui->ComboBox("Add Node", behSys->GetNodeTypeNames(), currentVal))
			{
				behSys->AddNode(*m_currentEditingTree, typeNames[currentVal]);
			}
		}
		dbgGui->EndWindow();
	}
}
