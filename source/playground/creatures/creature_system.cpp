#include "creature_system.h"
#include "core/log.h"
#include "engine/system_enumerator.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/debug_gui_system.h"
#include "engine/script_system.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "component_creature.h"

CreatureSystem::CreatureSystem()
{

}

CreatureSystem::~CreatureSystem()
{

}

void CreatureSystem::AddBehaviour(Engine::Tag tag, Behaviour b)
{
	m_behaviours.insert({ tag, b });
}

bool CreatureSystem::PreInit(Engine::SystemEnumerator& systemEnumerator)
{
	SDE_PROF_EVENT();

	m_entitySystem = (EntitySystem*)systemEnumerator.GetSystem("Entities");
	m_graphicsSystem = (GraphicsSystem*)systemEnumerator.GetSystem("Graphics");
	m_scriptSystem = (Engine::ScriptSystem*)systemEnumerator.GetSystem("Script");

	return true;
}

void CreatureSystem::AddScriptBehaviour(Engine::Tag tag, sol::protected_function fn)
{
	auto wrappedFn = [tag,fn](EntityHandle h, Creature& c) -> bool {
		sol::protected_function_result result = fn(h, c);
		if (!result.valid())
		{
			SDE_LOG("Failed to call behaviour function '%s'", tag.c_str());
		}
		bool r = result;
		return r;
	};
	AddBehaviour(tag, wrappedFn);
}

bool CreatureSystem::Initialise()
{
	SDE_PROF_EVENT();

	auto scripts = m_scriptSystem->Globals()["Creatures"].get_or_create<sol::table>();
	scripts["AddBehaviour"] = [this](Engine::Tag tag, sol::protected_function fn) {
		AddScriptBehaviour(tag, fn);
	};

	m_entitySystem->RegisterComponentType<Creature>();
	m_entitySystem->RegisterComponentUi<Creature>([](ComponentStorage& cs, EntityHandle e, Engine::DebugGuiSystem& dbg) {
		auto& c = *static_cast<Creature::StorageType&>(cs).Find(e);
		dbg.Text("State: ");
		dbg.SameLine();
		dbg.Text(c.GetState().c_str());
		c.SetEnergy(dbg.DragFloat("Energy", c.GetEnergy(), 0.1f, 0.0f));
		c.SetMoveSpeed(dbg.DragFloat("Movement speed", c.GetMoveSpeed(), 0.1f, 0.0f));
		c.SetWanderDistance(dbg.DragFloat("Wander distance", c.GetWanderDistance(), 0.1f, 0.0f));
		c.SetVisionRadius(dbg.DragFloat("Vision Radius", c.GetVisionRadius(), 0.1f, 0.0f));
		c.SetHungerThreshold(dbg.DragFloat("Hunger Threshold", c.GetHungerThreshold(), 0.1f, 0.0f));
		c.SetMovementCost(dbg.DragFloat("Movement Cost", c.GetMovementCost(), 0.1f, 0.0f));
		c.SetEatingSpeed(dbg.DragFloat("Eating Speed", c.GetEatingSpeed(), 0.1f, 0.0f));
		c.SetFullThreshold(dbg.DragFloat("Full Threshold", c.GetFullThreshold(), 0.1f, 0.0f));
		char text[256] = { '\0' };
		sprintf(text,"Visible Entities: %d", (int)c.GetVisibleEntities().size());
		dbg.Text(text);
		dbg.Text("Behaviours:");
		const auto& sb = c.GetBehaviours();
		for (const auto& state : sb)
		{
			if (dbg.TreeNode(state.first.c_str(), true))
			{
				for (const auto& b : state.second)
				{
					if (dbg.TreeNode(b.c_str()))
					{
						dbg.TreePop();
					}
				}
				dbg.TreePop();
			}
		}
	});

	return true;
}

void CreatureSystem::Reset()
{
	m_behaviours = {};
}

void CreatureSystem::UpdateVision(Creature& looker, glm::vec3 pos)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();

	std::vector<EntityHandle> visibleCreatures;
	world->ForEachComponent<Creature>([this, &transforms, pos, &looker, &visibleCreatures](Creature& c, EntityHandle owner) {
		if (&looker != &c)
		{
			auto trans = transforms->Find(owner);
			if (trans)
			{
				// use distance based on x and z axis only!
				auto p = trans->GetPosition();
				float d = glm::distance(p, pos);
				if (d < looker.GetVisionRadius())
				{
					visibleCreatures.push_back(owner);
				}
			}
		}
	});
	looker.SetVisibleEntities(std::move(visibleCreatures));
}

bool CreatureSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();

	// tick the sim
	{
		SDE_PROF_EVENT("UpdateVision");
		world->ForEachComponent<Creature>([this, &world, &transforms](Creature& c, EntityHandle owner) {
			if (c.GetVisionRadius() > 0.0f && c.GetEnergy() > 0.0f)
			{
				auto trans = transforms->Find(owner);
				if (trans)
				{
					UpdateVision(c, trans->GetPosition());
				}
			}
		});
	}

	// tick behaviours for current state
	{
		SDE_PROF_EVENT("RunBehaviours");
		world->ForEachComponent<Creature>([this, &world, &transforms](Creature& c, EntityHandle owner) {
			auto& state = c.GetState();
			auto foundBehaviours = c.GetBehaviours().find(state);
			if (foundBehaviours != c.GetBehaviours().end())
			{
				for (auto b : foundBehaviours->second)
				{
					auto registeredFn = m_behaviours.find(b);
					if (registeredFn != m_behaviours.end())
					{
						if (!registeredFn->second(owner, c))
						{
							break;	// we're done dealing with this state
						}
					}
				}
			}
		});
	}

	return true;
}

void CreatureSystem::Shutdown()
{
	m_behaviours.clear();
}