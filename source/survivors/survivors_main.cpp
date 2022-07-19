#include "survivors_main.h"
#include "engine/system_manager.h"
#include "engine/script_system.h"
#include "engine/debug_gui_system.h"
#include "engine/components/component_transform.h"
#include "engine/physics_system.h"
#include "entity/entity_system.h"
#include "player_component.h"
#include "monster_component.h"
#include "explosion_component.h"
#include "world_tile_system.h"
#include "engine/character_controller_system.h"
#include "engine/components/component_character_controller.h"
#include "engine/components/component_model_part_materials.h"
#include "entity_grid.h"

namespace Survivors
{
	SurvivorsMain::SurvivorsMain()
		: m_activeMonsterGrid({8.0f,8.0f})
	{

	}

	void SurvivorsMain::StartGame()
	{
		SDE_PROF_EVENT();
		auto physics = Engine::GetSystem<Engine::PhysicsSystem>("Physics");
		m_enemiesEnabled = true;
		physics->SetSimulationEnabled(true);
	}

	void SurvivorsMain::StopGame()
	{
		SDE_PROF_EVENT();
		auto physics = Engine::GetSystem<Engine::PhysicsSystem>("Physics");
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
		entities->RegisterComponentType<ExplosionComponent>();

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

	void SurvivorsMain::DoDamageInRadius(glm::vec3 pos, float radius, float damageAtCenter, float damageAtEdge)
	{
		const auto aabMin = pos - glm::vec3(radius, 0.0f, radius);
		const auto aabMax = pos + glm::vec3(radius, 0.0f, radius);
		m_activeMonsterGrid.ForEachNearby(aabMin, aabMax, [&](uint32_t& index) {
			auto& enemy = m_activeMonsters[index];
			const glm::vec3 explosionToEnemy = pos - enemy.m_targetPosition;
			const float d = glm::length(explosionToEnemy);
			const float enemyRadius = enemy.m_radius;
			if (d < (enemyRadius + radius))
			{
				float damage = damageAtCenter;	// todo scaling
				enemy.m_cc->SetCurrentHealth(enemy.m_cc->GetCurrentHealth() - damage);
				glm::vec2 knockBack = -glm::normalize(glm::vec2(explosionToEnemy.x, explosionToEnemy.z)) * damage * 2.0f;
				enemy.m_cc->AddKnockback(knockBack);
			}
		});
	}

	void SurvivorsMain::UpdateExplosions(float timeDelta)
	{
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();
		static auto explosionIterator = world->MakeIterator<ExplosionComponent, Transform>();
		std::vector<EntityHandle> finishedExplosions;
		explosionIterator.ForEach([&](ExplosionComponent& ec, Transform& t, EntityHandle e) {
			if (!ec.HasExploded())
			{
				DoDamageInRadius(t.GetPosition(), ec.GetDamageRadius(), ec.GetDamageAtCenter(), ec.GetDamageAtEdge());
				ec.SetHasExploded(true);
			}
			auto partMaterials = world->GetComponent<ModelPartMaterials>(e);
			if (partMaterials != nullptr && partMaterials->Materials().size() > 0)
			{
				auto& diffuseOpacity = partMaterials->Materials()[0].m_diffuseOpacity;
				diffuseOpacity.a = diffuseOpacity.a - ec.GetFadeoutSpeed() * timeDelta;
				if (diffuseOpacity.a <= 0.0f)
				{
					finishedExplosions.push_back(e);
				}
			}
		});
		
		for (auto it : finishedExplosions)
		{
			entities->GetWorld()->RemoveEntity(it);
		}
	}

	void SurvivorsMain::DoEnemyAvoidance(glm::vec3 playerPos, float timeDelta)
	{
		SDE_PROF_EVENT();
		for (int m = 0; m < m_activeMonsters.size(); ++m)
		{
			auto& monster = m_activeMonsters[m];
			auto posToPlayer = (playerPos - monster.m_targetPosition) * glm::vec3(1.0f, 0.0f, 1.0f);
			float distanceToPlayer = glm::length(posToPlayer);
			if (distanceToPlayer > 360)	// todo - min collision radius
			{
				continue;
			}
			const float collideRadius = monster.m_radius;
			const auto aabMin = monster.m_targetPosition - glm::vec3(collideRadius, 0.0f, collideRadius);
			const auto aabMax = monster.m_targetPosition + glm::vec3(collideRadius, 0.0f, collideRadius);
			for (int iteration = 0; iteration < m_avoidanceIterations; ++iteration)
			{
				int touched = 0;
				m_activeMonsterGrid.ForEachNearby(aabMin, aabMax, [&](uint32_t& index) {
					auto& neighbour = m_activeMonsters[index];
					if (neighbour.m_cc == monster.m_cc)
						return;
					const glm::vec3 neighbourPos = neighbour.m_targetPosition;
					glm::vec3 monsterToNearby = monster.m_targetPosition - neighbourPos;
					const float d = glm::length(monsterToNearby);
					const float neighbourRadius = neighbour.m_radius;
					if (d < (neighbourRadius + collideRadius))
					{
						++touched;
						monsterToNearby = -monsterToNearby / d;
						float shiftDistance = (neighbourRadius + collideRadius) - d;
						shiftDistance = shiftDistance * 0.2f;
						const glm::vec3 shift = monsterToNearby * shiftDistance;
						if (neighbourRadius < collideRadius)
						{
							neighbour.m_targetPosition = neighbour.m_targetPosition + shift;
						}
						else if (neighbourRadius == collideRadius)
						{
							monster.m_targetPosition = monster.m_targetPosition - shift * 0.5f;
							neighbour.m_targetPosition = neighbour.m_targetPosition + shift * 0.5f;
						}
						else
						{
							monster.m_targetPosition = monster.m_targetPosition - shift;
						}
					}
				});
				if (touched == 0)
				{
					break;
				}
			}
		}
		for (int m = 0; m < m_activeMonsters.size(); ++m)
		{
			auto& monster = m_activeMonsters[m];
			monster.m_transform->SetPosition(monster.m_targetPosition);
		}
	}

	void SurvivorsMain::CollectActiveAndDespawning(glm::vec3 playerPos, float timeDelta)
	{
		SDE_PROF_EVENT();
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();

		const auto rotation180 = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
		static auto monsterIterator = world->MakeIterator<MonsterComponent, Transform>();
		monsterIterator.ForEach([&](MonsterComponent& cc, Transform& t, EntityHandle e) {
			auto posToPlayer = (playerPos - t.GetPosition()) * glm::vec3(1.0f, 0.0f, 1.0f);
			float distanceToPlayer = glm::length(posToPlayer);
			if (distanceToPlayer >= cc.GetDespawnRadius() || cc.GetCurrentHealth() <= 0.0f)	// despawn if too far away or dead
			{
				m_monstersToDespawn.emplace_back(e);
			}
			else if (distanceToPlayer <= cc.GetVisionRadius())
			{
				glm::vec3 targetPosition = t.GetPosition();
				cc.SetKnockback(cc.GetKnockback() * cc.GetKnockbackFalloff());
				targetPosition += glm::vec3(cc.GetKnockback().x, 0.0f, cc.GetKnockback().y) * timeDelta;
				if (distanceToPlayer > (cc.GetCollideRadius() + 4.0f))	// todo - player radius
				{
					targetPosition = targetPosition + glm::normalize(posToPlayer) * timeDelta * cc.GetSpeed();
				}

				// look at the player while we are touching the  transform component
				auto lookTransform = glm::quatLookAt(posToPlayer, glm::vec3(0.0f, 1.0f, 0.0f));
				lookTransform = lookTransform * rotation180;
				t.SetOrientation(glm::normalize(lookTransform));

				// Add to the grid for later
				const float collideRadius = cc.GetCollideRadius();
				const auto aabMin = targetPosition - glm::vec3(collideRadius, 0.0f, collideRadius);
				const auto aabMax = targetPosition + glm::vec3(collideRadius, 0.0f, collideRadius);
				ActiveMonster am{ targetPosition, collideRadius, &cc, &t };
				m_activeMonsters.emplace_back(am);
				m_activeMonsterGrid.AddEntry(aabMin, aabMax, m_activeMonsters.size() - 1);
			}
		});
	}

	void SurvivorsMain::UpdateEnemies(float timeDelta)
	{
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();
		auto playerEntity = entities->GetFirstEntityWithTag("PlayerCharacter");
		auto playerTransform = world->GetComponent<Transform>(playerEntity);
		const glm::vec3 playerPos = playerTransform ? playerTransform->GetPosition() : glm::vec3(0.0f, 0.0f, 0.0f);

		static float tileSize = 8.0f;
		m_activeMonsterGrid.SetTileSize({ tileSize,tileSize });
		m_activeMonsterGrid.Reset();
		m_activeMonsters.clear();
		m_monstersToDespawn.clear();
		CollectActiveAndDespawning(playerPos, timeDelta);
		DoEnemyAvoidance(playerPos, timeDelta);
		UpdateExplosions(timeDelta);

		// despawn last as to not mess with component storage
		for (auto it : m_monstersToDespawn)
		{
			entities->GetWorld()->RemoveEntity(it);
		}
	}

	bool SurvivorsMain::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();
		auto worldTiles = Engine::GetSystem<Survivors::WorldTileSystem>("SurvivorsWorldTiles");

		if (m_enemiesEnabled)
		{
			UpdateEnemies(timeDelta);
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