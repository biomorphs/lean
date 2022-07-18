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

		if (m_enemiesEnabled)
		{
			auto playerEntity = entities->GetFirstEntityWithTag("PlayerCharacter");
			auto playerTransform = world->GetComponent<Transform>(playerEntity);
			std::vector<EntityHandle> monstersToDespawn;
			if (playerTransform != nullptr)
			{
				const glm::vec3 playerPos = playerTransform->GetPosition();
				auto monsterIterator = world->MakeIterator<MonsterComponent, Transform>();
				monsterIterator.ForEach([&](MonsterComponent& cc, Transform& t, EntityHandle e) {
					auto posToPlayer = playerPos - t.GetPosition();
					posToPlayer.y = 0.0f;	// ignore y component for now
					float distanceToPlayer = glm::length(posToPlayer);
					if (distanceToPlayer >= cc.GetDespawnRadius())	// despawn if too far away
					{
						monstersToDespawn.emplace_back(e);
						return;
					}
					if (distanceToPlayer <= cc.GetVisionRadius() && distanceToPlayer > 0.1f)	// move towards player if in vision range
					{
						auto lookTransform = glm::quatLookAt(posToPlayer, glm::vec3(0.0f, 1.0f, 0.0f));
						lookTransform = lookTransform * glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
						t.SetOrientation(glm::normalize(lookTransform));
						t.SetPosition(t.GetPosition() + glm::normalize(posToPlayer) * timeDelta * cc.GetSpeed());
					}
				});
				for (auto it : monstersToDespawn)
				{
					entities->GetWorld()->RemoveEntity(it);
				}
			}
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