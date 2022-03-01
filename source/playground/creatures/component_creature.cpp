#include "component_creature.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(Creature, 
	"SetAge", &Creature::SetAge,
	"GetAge", &Creature::GetAge,
	"SetMaxAge", &Creature::SetMaxAge,
	"GetMaxAge", &Creature::GetMaxAge,
	"SetMoveSpeed", &Creature::SetMoveSpeed,
	"GetMoveSpeed", &Creature::GetMoveSpeed,
	"SetVisionRadius", &Creature::SetVisionRadius,
	"GetVisionRadius", &Creature::GetVisionRadius,
	"GetVisibleEntities", &Creature::GetVisibleEntities,
	"SetHungerThreshold", &Creature::SetHungerThreshold,
	"GetHungerThreshold", &Creature::GetHungerThreshold,
	"SetEnergy", &Creature::SetEnergy,
	"GetEnergy", &Creature::GetEnergy,
	"AddBehaviour", &Creature::AddBehaviour,
	"SetState", &Creature::SetState,
	"GetState", &Creature::GetState,
	"GetMovementCost", &Creature::GetMovementCost,
	"SetMovementCost", &Creature::SetMovementCost,
	"SetEatingSpeed", &Creature::SetEatingSpeed,
	"GetEatingSpeed", &Creature::GetEatingSpeed,
	"SetFullThreshold", &Creature::SetFullThreshold,
	"GetFullThreshold", &Creature::GetFullThreshold,
	"SetWanderDistance", &Creature::SetWanderDistance,
	"GetWanderDistance", &Creature::GetWanderDistance,
	"AddFoodSourceTag", &Creature::AddFoodSourceTag,
	"GetFoodSourceTags", &Creature::GetFoodSourceTags,
	"AddVisionTag", &Creature::AddVisionTag,
	"GetVisionTags", &Creature::GetVisionTags,
	"GetMaxVisibleEntities", &Creature::GetMaxVisibleEntities,
	"SetMaxVisibleEntities", &Creature::SetMaxVisibleEntities,
	"AddFleeFromTag", &Creature::AddFleeFromTag,
	"GetFleeFromTags", &Creature::GetFleeFromTags,
	"GetBlackboard", &Creature::GetBlackboard
);

void Creature::AddBehaviour(Engine::Tag state, Engine::Tag behaviour)
{
	auto& behaviours = m_stateBehaviours[state];
	behaviours.push_back(behaviour);
}

COMPONENT_INSPECTOR_IMPL(Creature,Engine::DebugGuiSystem& dbg)
{
	auto fn = [&dbg](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e) {
		auto& c = *static_cast<Creature::StorageType&>(cs).Find(e);
		dbg.Text("State: ");
		dbg.SameLine();
		dbg.Text(c.GetState().c_str());
		c.SetEnergy(dbg.DragFloat("Energy", c.GetEnergy(), 0.1f, 0.0f));
		c.SetMoveSpeed(dbg.DragFloat("Movement speed", c.GetMoveSpeed(), 0.1f, 0.0f));
		c.SetMovementCost(dbg.DragFloat("Movement Cost", c.GetMovementCost(), 0.1f, 0.0f));
		c.SetWanderDistance(dbg.DragFloat("Wander distance", c.GetWanderDistance(), 0.1f, 0.0f));
		c.SetVisionRadius(dbg.DragFloat("Vision Radius", c.GetVisionRadius(), 0.1f, 0.0f));
		c.SetHungerThreshold(dbg.DragFloat("Hunger Threshold", c.GetHungerThreshold(), 0.1f, 0.0f));
		c.SetEatingSpeed(dbg.DragFloat("Eating Speed", c.GetEatingSpeed(), 0.1f, 0.0f));
		c.SetFullThreshold(dbg.DragFloat("Full Threshold", c.GetFullThreshold(), 0.1f, 0.0f));
		if (dbg.TreeNode("Food source tags"))
		{
			for (const auto& it : c.GetFoodSourceTags())
			{
				dbg.Text(it.c_str());
			}
			dbg.TreePop();
		}
		if (dbg.TreeNode("Flee From tags"))
		{
			for (const auto& it : c.GetFleeFromTags())
			{
				dbg.Text(it.c_str());
			}
			dbg.TreePop();
		}
		c.SetAge(dbg.DragFloat("Age", c.GetAge(), 0.1f, 0.0f));
		c.SetMaxAge(dbg.DragFloat("Max Age", c.GetMaxAge(), 0.1f, 0.0f));
		char text[256] = { '\0' };
		sprintf(text, "Visible Entities: %d", (int)c.GetVisibleEntities().size());
		dbg.Text(text);
		if (dbg.TreeNode("Blackboard"))
		{
			char text[256] = "";
			for (const auto& v : c.GetBlackboard()->GetInts())
			{
				sprintf(text, "%s: %d", v.first.c_str(), v.second);
				dbg.Text(text);
			}
			for (const auto& v : c.GetBlackboard()->GetFloats())
			{
				sprintf(text, "%s: %f", v.first.c_str(), v.second);
				dbg.Text(text);
			}
			for (const auto& v : c.GetBlackboard()->GetEntities())
			{
				sprintf(text, "%s: Entity %d", v.first.c_str(), v.second.GetID());
				dbg.Text(text);
			}
			for (const auto& v : c.GetBlackboard()->GetVectors())
			{
				sprintf(text, "%s: %f, %f, %f", v.first.c_str(), v.second.x, v.second.y, v.second.z);
				dbg.Text(text);
			}
			dbg.TreePop();
		}
		dbg.Text("Behaviours:");
		const auto& sb = c.GetBehaviours();
		for (const auto& state : sb)
		{
			if (dbg.TreeNode(state.first.c_str(), true))
			{
				for (const auto& b : state.second)
				{
					dbg.Text(b.c_str());
				}
				dbg.TreePop();
			}
		}
	};
	return fn;
}