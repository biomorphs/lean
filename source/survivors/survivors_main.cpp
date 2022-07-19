#include "survivors_main.h"
#include "engine/system_manager.h"
#include "engine/script_system.h"
#include "engine/debug_gui_system.h"
#include "engine/components/component_transform.h"
#include "engine/physics_system.h"
#include "entity/entity_system.h"
#include "player_component.h"
#include "monster_component.h"
#include "world_tile_system.h"
#include "engine/character_controller_system.h"
#include "engine/components/component_character_controller.h"
#include "entity_grid.h"

namespace Survivors
{
	void SurvivorsMain::StartGame()
	{
		SDE_PROF_EVENT();
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto physics = Engine::GetSystem<Engine::PhysicsSystem>("Physics");
		auto playerEntity = entities->GetFirstEntityWithTag("PlayerCharacter");
		auto playerCCT = entities->GetWorld()->GetComponent<CharacterController>(playerEntity);
		if (playerCCT)
		{
			playerCCT->SetEnabled(true);
		}
		m_enemiesEnabled = true;
		physics->SetSimulationEnabled(true);
	}

	void SurvivorsMain::StopGame()
	{
		SDE_PROF_EVENT();
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto physics = Engine::GetSystem<Engine::PhysicsSystem>("Physics");
		auto playerEntity = entities->GetFirstEntityWithTag("PlayerCharacter");
		auto playerCCT = entities->GetWorld()->GetComponent<CharacterController>(playerEntity);
		if (playerCCT)
		{
			playerCCT->SetEnabled(false);
		}
		m_enemiesEnabled = false;
		physics->SetSimulationEnabled(false);
	}

	bool SurvivorsMain::PostInit()
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		entities->RegisterComponentType<PlayerComponent>();
		entities->RegisterInspector<PlayerComponent>(PlayerComponent::MakeInspector());
		entities->RegisterComponentType<MonsterComponent>();
		entities->RegisterInspector<MonsterComponent>(MonsterComponent::MakeInspector());

		auto scripts = Engine::GetSystem<Engine::ScriptSystem>("Script");
		auto survivors = scripts->Globals()["Survivors"].get_or_create<sol::table>();
		survivors["SetWorldTileSpawnFn"] = [this](std::function<std::string(glm::ivec2)> fn) {
			m_worldTileSpawnFn = fn;
		};
		survivors["RemoveAllTiles"] = [this]() {
			auto worldTiles = Engine::GetSystem<Survivors::WorldTileSystem>("SurvivorsWorldTiles");
			worldTiles->DestroyAll();
		};
		survivors["GetWorldTileLoadRadius"] = [this]() {
			return m_tileLoadRadius;
		};
		survivors["SetWorldTileLoadRadius"] = [this](int r)	{
			m_tileLoadRadius = r;
		};
		survivors["SetEnemiesEnabled"] = [this](bool e) {
			m_enemiesEnabled = e;
		};
		survivors["StartGame"] = [this]() {
			StartGame();
		};
		survivors["StopGame"] = [this] {
			StopGame();
		};

		return true;
	}

	bool SurvivorsMain::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();
		auto worldTiles = Engine::GetSystem<Survivors::WorldTileSystem>("SurvivorsWorldTiles");
		
		struct ActiveMonster
		{
			glm::vec4 m_positionRadius;
			glm::vec3 m_targetPosition;
			MonsterComponent* m_cc;
			Transform* m_transform;
		};
		static WorldGrid<uint32_t> activeMonsterGrid({32.0f,32.0f});	// indexes into vector below
		static std::vector<ActiveMonster> activeMonsters;
		std::vector<EntityHandle> monstersToDespawn;
		activeMonsterGrid.Reset();
		activeMonsters.clear();

		// monster update first pass, collect active + despawning entities
		if (m_enemiesEnabled)
		{
			SDE_PROF_EVENT("Phase1");
			auto playerEntity = entities->GetFirstEntityWithTag("PlayerCharacter");
			auto playerTransform = world->GetComponent<Transform>(playerEntity);
			const glm::vec3 playerPos = playerTransform->GetPosition();
			const auto rotation180 = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
			static auto monsterIterator = world->MakeIterator<MonsterComponent, Transform>();
			monsterIterator.ForEach([&](MonsterComponent& cc, Transform& t, EntityHandle e) {
				auto posToPlayer = (playerPos - t.GetPosition()) * glm::vec3(1.0f,0.0f,1.0f);
				float distanceToPlayer = glm::length(posToPlayer);
				if (distanceToPlayer >= cc.GetDespawnRadius())	// despawn if too far away
				{
					monstersToDespawn.emplace_back(e);
				}
				else if (distanceToPlayer <= cc.GetVisionRadius())
				{
					glm::vec3 targetPosition = t.GetPosition();
					if (distanceToPlayer > cc.GetCollideRadius())
					{
						targetPosition  = t.GetPosition() + glm::normalize(posToPlayer) * timeDelta * cc.GetSpeed();
					}

					// look at the player
					auto lookTransform = glm::quatLookAt(posToPlayer, glm::vec3(0.0f, 1.0f, 0.0f));
					lookTransform = lookTransform * rotation180;
					t.SetOrientation(glm::normalize(lookTransform));

					// Add to the grid for later
					const float collideRadius = cc.GetCollideRadius();
					const auto pos = t.GetPosition();
					const auto aabMin = pos - glm::vec3(collideRadius, 0.0f, collideRadius);
					const auto aabMax = pos + glm::vec3(collideRadius, 0.0f, collideRadius);
					ActiveMonster am{ glm::vec4(pos, collideRadius), targetPosition , &cc, &t };
					activeMonsters.emplace_back(am);
					activeMonsterGrid.AddEntry(aabMin, aabMax, activeMonsters.size() - 1);
				}
			});

			// movement / collision pass
			{
				SDE_PROF_EVENT("Phase2");
				static std::vector<ActiveMonster> nearbyMonsters;
				for (int m = 0; m < activeMonsters.size(); ++m)
				{
					auto& monster = activeMonsters[m];
					auto posToPlayer = (playerPos - monster.m_targetPosition) * glm::vec3(1.0f, 0.0f, 1.0f);
					float distanceToPlayer = glm::length(posToPlayer);
					if (distanceToPlayer < monster.m_positionRadius.w)
					{
						continue;
					}

					const float collideRadius = monster.m_positionRadius.w;
					const auto aabMin = monster.m_targetPosition - glm::vec3(collideRadius, 0.0f, collideRadius);
					const auto aabMax = monster.m_targetPosition + glm::vec3(collideRadius, 0.0f, collideRadius);
					nearbyMonsters.clear();

					activeMonsterGrid.ForEachNearby(aabMin, aabMax, [&](uint32_t& index) {
						auto& neighbour = activeMonsters[index];
						if (neighbour.m_cc == monster.m_cc)
							return;
						const glm::vec3 neighbourPos = neighbour.m_targetPosition;
						glm::vec3 monsterToNearby = monster.m_targetPosition - neighbourPos;
						const float d = glm::length(monsterToNearby);
						const float neighbourRadius = neighbour.m_positionRadius.w;
						if (d < (neighbourRadius + collideRadius))
						{
							monsterToNearby = -monsterToNearby / d;
							float shiftDistance = (neighbourRadius + collideRadius) - d;
							shiftDistance = shiftDistance * 0.5f;	// damper on shift/frame
							const glm::vec3 shift = monsterToNearby * shiftDistance;
							if (neighbourRadius < collideRadius)
							{
								neighbour.m_targetPosition = neighbour.m_targetPosition + shift * 0.5f;
							}
							else if (neighbourRadius == collideRadius)
							{
								monster.m_targetPosition = monster.m_targetPosition - shift * 0.5f;
								neighbour.m_targetPosition = neighbour.m_targetPosition + shift * 0.5f;
							}
							else
							{
								monster.m_targetPosition = monster.m_targetPosition - shift * 0.5f;
							}
						}
					});
				}
			}
			for (int m = 0; m < activeMonsters.size(); ++m)
			{
				auto& monster = activeMonsters[m];
				monster.m_transform->SetPosition(monster.m_targetPosition);
			}
		}

		for (auto it : monstersToDespawn)
		{
			entities->GetWorld()->RemoveEntity(it);
		}

		// Load tiles around player, unload tiles far away
		auto playerIterator = world->MakeIterator<PlayerComponent, Transform>();
		playerIterator.ForEach([&](PlayerComponent& p, Transform& t, EntityHandle e) {
			if (m_worldTileSpawnFn != nullptr)
			{
				auto playerTilePos = worldTiles->PositionToTile(t.GetPosition());
				glm::ivec2 firstTile = playerTilePos - m_tileLoadRadius;
				glm::ivec2 lastTile = playerTilePos + m_tileLoadRadius;
				worldTiles->EnsureLoadedExclusive(firstTile, lastTile, m_worldTileSpawnFn);
			}
		});
		
		return true;
	}

	void SurvivorsMain::Shutdown()
	{
		SDE_PROF_EVENT();
		m_worldTileSpawnFn = nullptr;
	}
}