#pragma once
#include "engine/system.h"
#include "core/glm_headers.h"
#include "entity_grid.h"
#include "entity/entity_handle.h"
#include <functional>
#include <string>

class MonsterComponent;
class Transform;
class PlayerComponent;

namespace Survivors
{
	class SurvivorsMain : public Engine::System
	{
	public:
		SurvivorsMain();
		virtual bool PostInit();
		virtual bool Tick(float timeDelta);
		virtual void Shutdown();

	private:
		struct ActiveMonster
		{
			glm::vec3 m_targetPosition;
			float m_radius;
			MonsterComponent* m_cc;
			Transform* m_transform;
			EntityHandle m_entity;
		};

		void UpdateAttractors(PlayerComponent& playerCmp, float timeDelta);
		void DoDamageInRadius(glm::vec3 pos, float radius, float damageAtCenter, float damageAtEdge);
		void UpdateExplosions(float timeDelta);
		void DoEnemyAvoidance(glm::vec3 playerPos, float timeDelta);
		void CollectActiveAndDespawning(glm::vec3 playerPos, float timeDelta);
		void UpdateEnemies(EntityHandle player, PlayerComponent& playerCmp, glm::vec3 playerPos, float timeDelta);
		void KillEnemies(EntityHandle player, PlayerComponent& playerCmp);
		void CleanupDeadEnemies(glm::vec3 playerPos);
		void DamagePlayer(PlayerComponent& playerCmp, EntityHandle srcMonster, int damage);
		void UpdateAttackingMonsters(PlayerComponent &playerCmp, glm::vec3 playerPos);
		void StartGame();
		void StopGame();
		void LoadMainScene();
		void SpawnXPAt(EntityHandle player, glm::vec3 p, int xpToAdd);
		void SpawnMushroomAt(EntityHandle player, glm::vec3 p, int hpToAdd);
		void DamageMonster(ActiveMonster& monster, double curentTime, float damage, glm::vec2 knockback = glm::vec2(0.0f));

		bool m_firstFrame = true;
		double m_damagedMaterialTime = 0.25;			// time that a monster will show damaged material
		bool m_enemiesEnabled = false;
		bool m_attractorsEnabled = false;
		int m_avoidanceIterations = 1;				// increase for more stability but slower
		float m_avoidanceMaxDistance = 350.0f;
		WorldGrid<uint32_t> m_activeMonsterGrid;	// indexes into vector below
		std::vector<ActiveMonster> m_activeMonsters;
		std::vector<EntityHandle> m_monstersToDespawn;
		std::vector<EntityHandle> m_monstersToKill;
		EntityHandle m_xpTemplateEntity;
		EntityHandle m_mushroomTemplateEntity;
		int m_chanceToSpawnMushroom = 1;			// % chance to spawn mushroom on death
		int m_tileLoadRadius = 12;
		float m_monsterGridSize = 16.0f;
		double m_deadMonsterTimeout = 60.0;			// remove corpses after this time
		float m_deadMonsterCullRadius = 800.0f;		// remove corpses very far away
		std::function<std::string(glm::ivec2)> m_worldTileSpawnFn;
	};
}