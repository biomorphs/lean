#include "creature_system.h"
#include "core/log.h"
#include "engine/system_enumerator.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/debug_gui_system.h"
#include "engine/script_system.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_tags.h"
#include "behaviour_library.h"
#include "blackboard.h"

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
			sol::error err = result;
			std::string what = err.what();
			SDE_LOG("Failed to call behaviour function '%s'\n\t%s", tag.c_str(), what.c_str());
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

	AddBehaviour("move_to_target", BehaviourLibrary::MoveToTarget(*m_entitySystem, *m_graphicsSystem, "idle"));
	AddBehaviour("photosynthesize", BehaviourLibrary::Photosynthesize());
	AddBehaviour("die_at_max_age", BehaviourLibrary::DieAtMaxAge());
	AddBehaviour("die_at_zero_energy", BehaviourLibrary::DieAtZeroEnergy());
	AddBehaviour("flee_enemy", BehaviourLibrary::Flee(*m_entitySystem, *m_graphicsSystem));

	auto scripts = m_scriptSystem->Globals()["Creatures"].get_or_create<sol::table>();
	scripts["AddBehaviour"] = [this](Engine::Tag tag, sol::protected_function fn) {
		AddScriptBehaviour(tag, fn);
	};

	m_scriptSystem->Globals().new_usertype<Blackboard>("Blackboard", sol::constructors<Blackboard()>(),
		"ContainsInt", &Blackboard::ContainsInt,
		"SetInt", &Blackboard::SetInt,
		"GetInt", &Blackboard::GetInt,
		"RemoveInt", &Blackboard::RemoveInt,
		"ContainsEntity", &Blackboard::ContainsEntity,
		"SetEntity", &Blackboard::SetEntity,
		"GetEntity", &Blackboard::GetEntity,
		"RemoveEntity", &Blackboard::RemoveEntity,
		"ContainsVector", &Blackboard::ContainsVector,
		"SetVector", &Blackboard::SetVector3,
		"GetVector", &Blackboard::GetVector,
		"RemoveVector", &Blackboard::RemoveVector
	);

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
		sprintf(text,"Visible Entities: %d", (int)c.GetVisibleEntities().size());
		dbg.Text(text);
		if (dbg.TreeNode("Blackboard"))
		{
			char text[256] = "";
			for (const auto& v : c.GetBlackboard()->GetInts())
			{
				sprintf(text, "%s: %d", v.first.c_str(), v.second);
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
	auto tags = world->GetAllComponents<Tags>();
	using VisRecord = std::pair<EntityHandle, float>;
	std::vector<VisRecord> visibleCreatures;
	std::vector<EntityHandle> visibleEntities;
	{
		SDE_PROF_EVENT("CollectAllInRadius");
		world->ForEachComponent<Creature>([this, &transforms, &tags, pos, &looker, &visibleCreatures](Creature& c, EntityHandle owner) {
			if (&looker != &c)
			{
				bool canSeeObject = true;
				if (looker.GetVisionTags().size() > 0)
				{
					auto foundTags = tags->Find(owner);
					if (foundTags != nullptr)
					{
						bool tagMatch = false;
						for (const auto& t : looker.GetVisionTags())
						{
							tagMatch |= foundTags->ContainsTag(t);
							if (tagMatch)
								break;
						}
						canSeeObject = tagMatch;
					}
				}
				if (canSeeObject)
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
		static double s_visionMaxTime = 0.008f;		// spend 8ms max on visibility per frame
		if (s_lastCreatureUpdated >= s_allCreaturesToUpdate.size())
		{
			// populate creature list to timeslice
			s_allCreaturesToUpdate.clear();
			world->ForEachComponent<Creature>([this](Creature& c, EntityHandle owner) {
				if (c.GetVisionRadius() > 0.0f && c.GetEnergy() > 0.0f && c.GetState() != "dead")
				{
					s_allCreaturesToUpdate.push_back({ &c,owner });
				}
			});
			s_lastCreatureUpdated = 0;
		}
		int toUpdate = s_lastCreatureUpdated;
		int visionUpdates = 0;
		Core::Timer visionTimer;
		const double startTime = visionTimer.GetSeconds();
		while ((visionTimer.GetSeconds() - startTime < s_visionMaxTime) && toUpdate < s_allCreaturesToUpdate.size())
		{
			auto& c = s_allCreaturesToUpdate[toUpdate];
			auto trans = transforms->Find(c.second);
			if (trans)
			{
				UpdateVision(*c.first, trans->GetPosition());
				++visionUpdates;
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
			if (c.GetEnergy() > 0 && c.GetState() != "dead")
			{
				c.SetAge(c.GetAge() + timeDelta);	// age is special, increment it here
			}
			auto& state = c.GetState();
			auto foundBehaviours = c.GetBehaviours().find(state);
			if (foundBehaviours != c.GetBehaviours().end())
			{
				for (auto b : foundBehaviours->second)
				{
					auto registeredFn = m_behaviours.find(b);
					if (registeredFn != m_behaviours.end())
					{
						char debugName[1024] = { '\0' };
						sprintf_s(debugName, "RunBehaviour %s_%s", state.c_str(), b.c_str());
						SDE_PROF_EVENT_DYN(debugName);
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