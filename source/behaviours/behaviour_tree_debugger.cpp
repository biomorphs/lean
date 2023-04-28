#include "behaviour_tree_debugger.h"
#include "behaviour_tree_system.h"
#include "behaviour_tree_renderer.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"
#include "entity/entity_system.h"

namespace Behaviours
{
	void ShowBlackboard(BehaviourTreeInstance* inst)
	{
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		bool openWindow = true;
		dbgGui->BeginWindow(openWindow, "Blackboard");
		for (auto& ints : inst->m_bb.GetInts())
		{
			ints.second = dbgGui->InputInt(ints.first.c_str(), ints.second, 1);
		}
		for (auto& f : inst->m_bb.GetFloats())
		{
			f.second = dbgGui->InputFloat(f.first.c_str(), f.second, 1);
		}
		for (auto& v : inst->m_bb.GetVectors())
		{
			v.second = dbgGui->DragVector(v.first.c_str(), v.second, 0.1f);
		}
		for (auto& v : inst->m_bb.GetEntities())
		{
			std::string entName = entities->GetEntityNameWithTags(v.second);
			dbgGui->TextInput(v.first.c_str(), entName);
		}
		dbgGui->EndWindow();
	}

	void BehaviourTreeDebugger::DebugTreeInstance(BehaviourTreeInstance* inst)
	{
		BehaviourTreeRenderer drawTreeInstance;
		drawTreeInstance.DrawTree(*inst->m_tree, inst);
		ShowBlackboard(inst);
	}
}
