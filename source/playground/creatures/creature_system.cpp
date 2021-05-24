#include "creature_system.h"
#include "core/log.h"
#include "core/thread.h"
#include "engine/system_manager.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/debug_gui_system.h"
#include "engine/script_system.h"
#include "entity/entity_system.h"
#include "engine/job_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_tags.h"
#include "behaviour_library.h"
#include "blackboard.h"

glm::vec2 c_gridCellSize = {128.0f,128.0f};

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

bool CreatureSystem::PreInit(Engine::SystemManager& manager)
{
	SDE_PROF_EVENT();

	m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");
	m_graphicsSystem = (GraphicsSystem*)manager.GetSystem("Graphics");
	m_scriptSystem = (Engine::ScriptSystem*)manager.GetSystem("Script");
	m_jobSystem = (Engine::JobSystem*)manager.GetSystem("Jobs");

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
		"ContainsFloat", &Blackboard::ContainsFloat,
		"SetFloat", &Blackboard::SetFloat,
		"GetFloat", &Blackboard::GetFloat,
		"RemoveFloat", &Blackboard::RemoveFloat,
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
	});

	return true;
}

void CreatureSystem::Reset()
{
	m_behaviours = {};
}

uint64_t VisHash(const glm::vec3& p)
{
	const auto i = glm::floor(p) / glm::vec3(c_gridCellSize.x, 1.0f, c_gridCellSize.y);
	auto gridcell = (glm::ivec3)i;
	return (uint64_t)(gridcell.x) << 32 | (uint64_t)gridcell.z;
}

uint64_t VisHash(const glm::ivec2& p)
{
	auto x = (((uint64_t)p.x) & 0x00000000ffffffff) << 32;
	auto y = ((uint64_t)p.y) & 0x00000000ffffffff;
	return x | y;
}

void CreatureSystem::UpdateVision(const std::vector<VisibilityRecord>& allCreatures, robin_hood::unordered_map<uint64_t, std::vector<VisibilityRecord>>& grid, Creature& looker, glm::vec3 pos)
{
	SDE_PROF_EVENT();

	using VisRecord = std::pair<VisibilityRecord, float>;	// keep distance to creature for later
	std::vector<VisRecord> visibleCreatures;
	visibleCreatures.reserve(32);

	const auto visionRadiusSq = looker.GetVisionRadius() * looker.GetVisionRadius();
	const auto minBounds = glm::ivec3(glm::floor(pos - looker.GetVisionRadius()) / glm::vec3(c_gridCellSize.x, 1.0f, c_gridCellSize.y));
	const auto maxBounds = glm::ivec3(glm::floor(pos + looker.GetVisionRadius()) / glm::vec3(c_gridCellSize.x, 1.0f, c_gridCellSize.y));
	const auto tagCount = looker.GetVisionTags().size();
	for (int gridx = minBounds.x; gridx <= maxBounds.x; ++gridx)
	{
		for (int gridz = minBounds.z; gridz <= maxBounds.z; ++gridz)
		{
			const uint64_t gridHash = VisHash(glm::ivec2(gridx, gridz));
			const auto foundCell = grid.find(gridHash);
			if (foundCell != grid.end())
			{
				for (const auto& record : foundCell->second)
				{
					const auto c = std::get<0>(record);
					if (&looker != c)
					{
						float d = glm::distance2(std::get<3>(record), pos);		// distance squared
						if (d < visionRadiusSq)
						{
							bool canSeeObject = tagCount == 0;
							const auto tags = std::get<1>(record);
							if (tags && tagCount > 0)
							{
								for (const auto& t : looker.GetVisionTags())
								{
									canSeeObject |= tags->ContainsTag(t);
									if (canSeeObject)
										break;
								}
							}
							if (canSeeObject)
							{
								visibleCreatures.push_back({ record, d });
							}
						}
					}
				}
			}
		}
	}

	std::sort(visibleCreatures.begin(), visibleCreatures.end(), [](const VisRecord& s0, const VisRecord& s1)
		{
			return std::get<1>(s0) < std::get<1>(s1);
		});
	std::vector<EntityHandle> visibleEntities;
	auto entitiesToAdd = glm::min((uint32_t)visibleCreatures.size(), looker.GetMaxVisibleEntities());
	visibleEntities.reserve(entitiesToAdd);
	for (int i = 0; i < entitiesToAdd; ++i)
	{
		visibleEntities.push_back(std::get<2>(std::get<0>(visibleCreatures[i])));
	}
	looker.SetVisibleEntities(std::move(visibleEntities));
}

void CreatureSystem::UpdateVision(const std::vector<VisibilityRecord>& allCreatures, Creature& looker, glm::vec3 pos)
{
	SDE_PROF_EVENT();

	using VisRecord = std::pair<VisibilityRecord, float>;	// keep distance to creature for later
	std::vector<VisRecord> visibleCreatures;
	visibleCreatures.reserve(allCreatures.size());
	const auto tagCount = looker.GetVisionTags().size();
	for (const auto& oneCreature : allCreatures)
	{
		auto [c, tags, owner, testpos] = oneCreature;
		if (&looker != c)
		{
			bool canSeeObject = true;
			if (tagCount > 0)
			{
				if (tags)
				{
					bool tagMatch = false;
					for (const auto& t : looker.GetVisionTags())
					{
						tagMatch |= tags->ContainsTag(t);
						if (tagMatch)
							break;
					}
					canSeeObject = tagMatch;
				}
			}
			if (canSeeObject)
			{
				// use distance based on x and z axis only!
				float d = glm::distance(testpos, pos);
				if (d < looker.GetVisionRadius())
				{
					visibleCreatures.push_back({ oneCreature, d });
				}
			}
		}
	}

	std::sort(visibleCreatures.begin(), visibleCreatures.end(), [](const VisRecord& s0, const VisRecord& s1)
	{
		return std::get<1>(s0) < std::get<1>(s1);
	});
	std::vector<EntityHandle> visibleEntities;
	auto entitiesToAdd = glm::min((uint32_t)visibleCreatures.size(), looker.GetMaxVisibleEntities());
	visibleEntities.reserve(entitiesToAdd);
	for (int i = 0; i < entitiesToAdd; ++i)
	{
		visibleEntities.push_back(std::get<2>(std::get<0>(visibleCreatures[i])));
	}
	looker.SetVisibleEntities(std::move(visibleEntities));
}

void CreatureSystem::UpdateVision(Creature& looker, glm::vec3 pos)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();
	auto tags = world->GetAllComponents<Tags>();
	using VisRecord = std::pair<EntityHandle, float>;
	std::vector<VisRecord> visibleCreatures;
	visibleCreatures.reserve(32);
	std::vector<EntityHandle> visibleEntities;
	const auto tagCount = looker.GetVisionTags().size();
	world->ForEachComponent<Creature>([this, &transforms, &tagCount , &tags, pos, &looker, &visibleCreatures](Creature& c, EntityHandle owner) {
		if (&looker != &c)
		{
			bool canSeeObject = true;
			if (tags && tagCount > 0)
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
	std::sort(visibleCreatures.begin(), visibleCreatures.end(), [](const VisRecord& s0, const VisRecord& s1)
	{
		return s0.second < s1.second;
	});
	auto maxToCollect = glm::min((uint32_t)visibleCreatures.size(), looker.GetMaxVisibleEntities());
	visibleEntities.reserve(maxToCollect);
	for (int i = 0; i < maxToCollect; ++i)
	{
		visibleEntities.push_back(visibleCreatures[i].first);
	}
	looker.SetVisibleEntities(std::move(visibleEntities));
}

bool CreatureSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();
	auto alltags = world->GetAllComponents<Tags>();

	// Populate the full list of creatures along with their positions and tags
	// Also populate of creatures that need vis queries
	robin_hood::unordered_map<uint64_t, std::vector<VisibilityRecord>> grid;	// indexes into allcreatures
	std::vector<VisibilityRecord> allCreatures;
	std::vector<VisibilityRecord> creaturesToUpdate;
	{
		SDE_PROF_EVENT("CollectAll");
		allCreatures.reserve(8000);
		creaturesToUpdate.reserve(2000);
		world->ForEachComponent<Creature>([&allCreatures, &grid, &creaturesToUpdate, &alltags, &transforms](Creature& c, EntityHandle owner) {
			auto transform = transforms->Find(owner);
			auto tags = alltags->Find(owner);
			if (transform != nullptr)
			{
				auto hash = VisHash(transform->GetPosition());
				VisibilityRecord r = { &c, tags, owner, transform->GetPosition() };
				allCreatures.push_back(r);
				grid[hash].push_back(r);
				if (c.GetVisionRadius() > 0.0f && c.GetEnergy() > 0.0f && c.GetState() != "dead")
				{
					creaturesToUpdate.push_back(r);
				}
			}
		});
	}
	const bool s_useJobs = true;
	if(!s_useJobs)
	{
		SDE_PROF_EVENT("TestVisNaive");
		for (const auto& c : creaturesToUpdate)
		{
			auto creature = std::get<0>(c);
			UpdateVision(allCreatures, grid, *creature, std::get<3>(c));
		}
	}
	else
	{
		SDE_PROF_EVENT("TestVisJobs");
		const int c_testsPerJob = 32;
		std::atomic<int> jobsRemaining = 0;
		for (int c = 0; c < creaturesToUpdate.size(); c += c_testsPerJob)
		{
			int startIndex = c;
			int endIndex = std::min(c + c_testsPerJob, (int)creaturesToUpdate.size());
			auto visJob = [this,&allCreatures,&grid,&creaturesToUpdate,&jobsRemaining,startIndex,endIndex](void*)
			{
				SDE_PROF_EVENT("VisJob");
				for (int index = startIndex; index < endIndex; ++index)
				{
					auto creature = std::get<0>(creaturesToUpdate[index]);
					UpdateVision(allCreatures, grid, *creature, std::get<3>(creaturesToUpdate[index]));
				}
				jobsRemaining--;
			};
			jobsRemaining++;
			m_jobSystem->PushJob(visJob);
		}
		{
			SDE_PROF_STALL("WaitForVisJobs");
			while (jobsRemaining > 0)
			{
				Core::Thread::Sleep(0);
			}
		}
	}

	// tick all behaviours for current state
	{
		SDE_PROF_EVENT("RunBehaviours");
		world->ForEachComponent<Creature>([this, &world, &transforms, timeDelta](Creature& c, EntityHandle owner) {
			if (c.GetEnergy() > 0 && c.GetState() != "dead")
			{
				c.SetAge(c.GetAge() + timeDelta);	// age is special, increment it here for convenience
			}
			auto& state = c.GetState();
			auto foundBehaviours = c.GetBehaviours().find(state);
			if (foundBehaviours != c.GetBehaviours().end())
			{
				for (const auto& b : foundBehaviours->second)
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