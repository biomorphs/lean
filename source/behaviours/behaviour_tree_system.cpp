#include "behaviour_tree_system.h"
#include "behaviour_tree.h"
#include "behaviour_tree_editor.h"
#include "behaviour_tree_renderer.h"
#include "behaviour_tree_instance.h"
#include "basic_nodes.h"
#include "core/log.h"
#include "core/profiler.h"
#include "engine/graphics_system.h"
#include "engine/texture_manager.h"
#include "engine/2d_render_context.h"
#include "engine/system_manager.h"
#include "engine/text_system.h"
#include "engine/debug_gui_system.h"

namespace Behaviours
{
	BehaviourTreeRenderer g_treeRenderer;
	BehaviourTree g_testTree;
	BehaviourTreeInstance g_testInstance(&g_testTree);
	BehaviourTreeEditor g_treeEditor;

	uint16_t BehaviourTreeSystem::AddNode(BehaviourTree& tree, std::string nodeTypeName)
	{
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

		for (int f = 0; f < m_nodeFactories.size(); ++f)
		{
			m_nodeFactoryNames.push_back(m_nodeFactories[f].m_nodeName);
		}

		g_testInstance.Reset();

		return true;
	}

	bool BehaviourTreeSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		g_treeEditor.ShowEditor(g_testTree, m_nodeFactoryNames, ([&]() {
			g_testInstance.Reset();
		}));
		g_treeRenderer.DrawTree(g_testTree, &g_testInstance);

		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		bool windowOpen = true;
		dbgGui->BeginWindow(windowOpen, "Debugger");
		if (dbgGui->Button("Reset"))
		{
			g_testInstance.Reset();
		}
		if (dbgGui->Button("Tick"))
		{
			g_testInstance.Tick();
		}
		dbgGui->EndWindow();

		return true;
	}

	void BehaviourTreeSystem::Shutdown()
	{
		SDE_PROF_EVENT();
	}

}