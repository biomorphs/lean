#include "creature_system.h"
#include "core/log.h"
#include "engine/system_enumerator.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/debug_gui_system.h"
#include "engine/script_system.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "behaviour_library.h"

CreatureSystem::CreatureSystem()
{

}

CreatureSystem::~CreatureSystem()
{

}

void CreatureSystem::AddBehaviour(Engine::Tag tag, Creature::Behaviour b)
{
	m_behaviours[tag] = b;
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
	auto wrappedFn = [tag,fn](EntityHandle h, Creature& c, float delta) -> bool {
		sol::protected_function_result result = fn(h, c, delta);
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

	// Register the default behaviours
	// These can be attached to any state of any creature
	// Anything generic enough should be implemented in c++

	// The built-in states are:
	// 'idle' - should be obvious, used as default state
	// 'dying' - transition state used as a trigger so behaviours can do stuff on death
	//		(nothing will die by default, you must handle the state transition yourself!)

	// Move to target, transition to idle on completion
	AddBehaviour("move_to_target", BehaviourLibrary::MoveToTarget(*m_entitySystem, *m_graphicsSystem, "idle"));
	AddBehaviour("photosynthesize", BehaviourLibrary::Photosynthesize());
	AddBehaviour("die_at_max_age", BehaviourLibrary::DieAtMaxAge());
	AddBehaviour("die_at_zero_energy", BehaviourLibrary::DieAtZeroEnergy());

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
				dbg.TreeNode(it.c_str());
			}
			dbg.TreePop();
		}
		c.SetAge(dbg.DragFloat("Age", c.GetAge(), 0.1f, 0.0f));
		c.SetMaxAge(dbg.DragFloat("Max Age", c.GetMaxAge(), 0.1f, 0.0f));
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
	using VisRecord = std::pair<EntityHandle, float>;
	std::vector<VisRecord> visibleCreatures;
	std::vector<EntityHandle> visibleEntities;
	{
		SDE_PROF_EVENT("CollectAllInRadius");
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
						visibleCreatures.push_back({ owner,d });
					}
				}
			}
		});
	}
	{
		SDE_PROF_EVENT("SortAndCollect");
		std::sort(visibleCreatures.begin(), visibleCreatures.end(), [](const VisRecord& s0, const VisRecord& s1)
		{
			return s0.second < s1.second;
		});
		visibleEntities.reserve(visibleCreatures.size());
		for (int i = 0; i < glm::min((uint32_t)visibleCreatures.size(), looker.GetMaxVisibleEntities()); ++i)
		{
			visibleEntities.push_back(visibleCreatures[i].first);
		}
	}
	looker.SetVisibleEntities(std::move(visibleEntities));
}

bool CreatureSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();

	// time slicing for vision updates
	{
		SDE_PROF_EVENT("UpdateVision");
		static std::vector<std::pair<Creature*, EntityHandle>> s_allCreaturesToUpdate;
		static int s_lastCreatureUpdated = 0;
		static int s_updatesPerTick = 50;
		if (s_lastCreatureUpdated >= s_allCreaturesToUpdate.size())
		{
			// populate creature list to timeslice
			s_allCreaturesToUpdate.clear();
			world->ForEachComponent<Creature>([this](Creature& c, EntityHandle owner) {
				s_allCreaturesToUpdate.push_back({ &c,owner });
				});
			s_lastCreatureUpdated = 0;
		}
		int toUpdate = s_lastCreatureUpdated;
		int visionUpdates = 0;
		while ((visionUpdates < s_updatesPerTick) && toUpdate < s_allCreaturesToUpdate.size())
		{
			auto& c = s_allCreaturesToUpdate[toUpdate];
			if (c.first->GetVisionRadius() > 0.0f && c.first->GetEnergy() > 0.0f)
			{
				auto trans = transforms->Find(c.second);
				if (trans)
				{
					UpdateVision(*c.first, trans->GetPosition());
					++visionUpdates;
				}
			}
			toUpdate++;
		}
		s_lastCreatureUpdated = toUpdate;
	}

	// tick all behaviours for current state
	{
		SDE_PROF_EVENT("RunBehaviours");
		world->ForEachComponent<Creature>([this, &world, &transforms, timeDelta](Creature& c, EntityHandle owner) {
			// If something is shared across all creatures, do it here. DO NOT ABUSE IT
			c.SetAge(c.GetAge() + timeDelta);	// age is special, increment it here
			auto& state = c.GetState();
			auto foundBehaviours = c.GetBehaviours().find(state);
			if (foundBehaviours != c.GetBehaviours().end())
			{
				for (auto b : foundBehaviours->second)
				{
					auto registeredFn = m_behaviours.find(b);
					if (registeredFn != m_behaviours.end())
					{
						//char debugName[1024] = { '\0' };
						//sprintf_s(debugName, "RunBehaviour %s_%s", state.c_str(), b.c_str());
						//SDE_PROF_EVENT_DYN(debugName);
						if (!registeredFn->second(owner, c, timeDelta))
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