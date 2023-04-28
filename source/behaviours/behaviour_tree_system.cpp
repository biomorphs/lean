#include "behaviour_tree_system.h"
#include "behaviour_tree.h"
#include "behaviour_tree_editor.h"
#include "behaviour_tree_instance.h"
#include "behaviour_tree_debugger.h"
#include "basic_nodes.h"
#include "core/file_io.h"
#include "core/log.h"
#include "core/profiler.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"

namespace Behaviours
{
	BehaviourTreeSystem::~BehaviourTreeSystem()
	{
	}

	void BehaviourTreeSystem::OnTreeModified(std::string_view path)
	{
		SDE_PROF_EVENT();
		std::string pathStr(path);
		auto loadedTree = m_loadedTrees.find(pathStr);
		if (loadedTree != m_loadedTrees.end())
		{
			BehaviourTree* oldTreePtr = loadedTree->second.get();
			BehaviourTree* newTreePtr = oldTreePtr;

			std::string fileText;
			if (Core::LoadTextFromFile(path.data(), fileText))
			{
				nlohmann::json treeJson;
				{
					SDE_PROF_EVENT("ParseJson");
					treeJson = nlohmann::json::parse(fileText);
				}
				auto newTree = std::make_unique<BehaviourTree>();
				newTree->Serialise(treeJson, Engine::SerialiseType::Read);
				newTreePtr = newTree.get();
				loadedTree->second = std::move(newTree);
			}

			// reset all connected instances
			for (auto& inst : m_activeInstances)
			{
				if (inst->m_tree == oldTreePtr)
				{
					inst->m_tree = newTreePtr;
					inst->Reset();
				}
			}
		}
	}

	BehaviourTree* BehaviourTreeSystem::LoadTree(std::string_view path)
	{
		SDE_PROF_EVENT();
		std::string pathStr(path);
		auto loadedTree = m_loadedTrees.find(pathStr);
		if (loadedTree == m_loadedTrees.end())
		{
			std::string fileText;
			if (!Core::LoadTextFromFile(path.data(), fileText))
			{
				return nullptr;
			}
			nlohmann::json treeJson;
			{
				SDE_PROF_EVENT("ParseJson");
				treeJson = nlohmann::json::parse(fileText);
			}
			auto newTree = std::make_unique<BehaviourTree>();
			newTree->Serialise(treeJson, Engine::SerialiseType::Read);
			auto newTreePtr = newTree.get();
			m_loadedTrees[pathStr] = std::move(newTree);
			return newTreePtr;
		}
		else
		{
			return loadedTree->second.get();
		}
	}

	BehaviourTreeInstance* BehaviourTreeSystem::CreateTreeInstance(std::string_view path)
	{
		SDE_PROF_EVENT();
		BehaviourTree* tree = LoadTree(path);
		if (tree)
		{
			auto newInstance = std::make_unique<BehaviourTreeInstance>(tree);
			auto newInstancePtr = newInstance.get();
			newInstancePtr->m_name = std::string(path);
			m_activeInstances.push_back(std::move(newInstance));
			return newInstancePtr;
		}
		return nullptr;
	}

	void BehaviourTreeSystem::DestroyInstance(BehaviourTreeInstance* instance)
	{
		SDE_PROF_EVENT();
		auto found = std::find_if(m_activeInstances.begin(), m_activeInstances.end(), [instance](const std::unique_ptr<BehaviourTreeInstance>& ti) {
			return ti.get() == instance;
		});
		if (found != m_activeInstances.end())
		{
			if (m_debuggingInstance == instance)
			{
				m_debuggingInstance = nullptr;
			}
			m_activeInstances.erase(found);
		}
	}

	uint16_t BehaviourTreeSystem::AddNode(BehaviourTree& tree, std::string nodeTypeName)
	{
		SDE_PROF_EVENT();
		auto foundFactory = std::find_if(m_nodeFactories.begin(), m_nodeFactories.end(), [&nodeTypeName](const NodeFactory& nf) {
			return nf.m_nodeName == nodeTypeName;
		});
		if (foundFactory != m_nodeFactories.end())
		{
			return tree.AddNode(foundFactory->m_fn());
		}
		else
		{
			return -1;
		}
	}

	void BehaviourTreeSystem::RegisterNodeType(std::string_view typestr, std::function<std::unique_ptr<Node>()> factory)
	{
		SDE_PROF_EVENT();
		m_nodeFactories.push_back({	std::string(typestr), factory });

		m_nodeFactoryNames.clear();
		for (int f = 0; f < m_nodeFactories.size(); ++f)
		{
			m_nodeFactoryNames.push_back(m_nodeFactories[f].m_nodeName);
		}
	}

	bool BehaviourTreeSystem::Initialise()
	{
		SDE_PROF_EVENT();

		m_nodeFactories.push_back({
			"Sequence", []() {
				return std::make_unique<SequenceNode>();
			}
		});

		m_nodeFactories.push_back({
			"Selector", []() {
				return std::make_unique<SelectorNode>();
			}
		});

		m_nodeFactories.push_back({
			"Repeater", []() {
				return std::make_unique<RepeaterNode>();
			}
		});

		m_nodeFactories.push_back({
			"Succeeder", []() {
				return std::make_unique<SucceederNode>();
			}	
		});

		m_nodeFactories.push_back({
			"FloatComparison", []() {
				return std::make_unique<CompareFloatsNode>();
			}
		});

		for (int f = 0; f < m_nodeFactories.size(); ++f)
		{
			m_nodeFactoryNames.push_back(m_nodeFactories[f].m_nodeName);
		}

		return true;
	}

	bool BehaviourTreeSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");

		Engine::MenuBar mainMenu;
		auto& menu = mainMenu.AddSubmenu(ICON_FK_LEAF " Behaviours");
		menu.AddItem("Show Debugger", [this]() {
			m_debuggerEnabled = true;
		});
		dbgGui->MainMenuBar(mainMenu);

		if (m_debuggerEnabled)
		{
			dbgGui->BeginWindow(m_debuggerEnabled, "Debugger");
			for (int i = 0; i < m_activeInstances.size(); ++i)
			{
				std::string instanceTxt = std::to_string(i) + ":" + m_activeInstances[i]->m_name;
				if (m_debuggingInstance == m_activeInstances[i].get())
				{
					instanceTxt = std::string("*** ") + instanceTxt;
				}
				if (dbgGui->Button(instanceTxt.c_str()))
				{
					m_debuggingInstance = m_activeInstances[i].get();
				}
			}
			dbgGui->EndWindow();
			if (m_debuggingInstance)
			{
				BehaviourTreeDebugger dbg;
				dbg.DebugTreeInstance(m_debuggingInstance);
			}
		}

		return true;
	}

	void BehaviourTreeSystem::Shutdown()
	{
		SDE_PROF_EVENT();
	}

}