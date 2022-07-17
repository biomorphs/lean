#include "survivors_main.h"
#include "engine/system_manager.h"
#include "engine/script_system.h"
#include "engine/debug_gui_system.h"
#include "engine/components/component_transform.h"
#include "entity/entity_system.h"
#include "player_component.h"
#include "world_tile_system.h"
#include "engine/character_controller_system.h"
#include "engine/components/component_character_controller.h"

namespace Survivors
{
	bool SurvivorsMain::PostInit()
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		entities->RegisterComponentType<PlayerComponent>();
		entities->RegisterInspector<PlayerComponent>(PlayerComponent::MakeInspector());

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
			if (playerTransform != nullptr)
			{
				const glm::vec3 playerPos = playerTransform->GetPosition();
				auto characterControllerIterator = world->MakeIterator<CharacterController, Transform>();
				characterControllerIterator.ForEach([&](CharacterController& cc, Transform& t, EntityHandle e) {
					if (e.GetID() != playerEntity.GetID())
					{
						cc.SetEnabled(true);
						auto posToPlayer = playerPos - t.GetPosition();
						t.SetPosition(t.GetPosition() + glm::normalize(posToPlayer) * timeDelta * 8.0f);
					}
				});
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
		m_worldTileSpawnFn = nullptr;
	}
}