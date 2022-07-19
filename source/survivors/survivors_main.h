#pragma once
#include "engine/system.h"
#include "core/glm_headers.h"
#include "entity_grid.h"
#include "entity/entity_handle.h"
#include <functional>
#include <string>

class MonsterComponent;
class Transform;
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
		void DoDamageInRadius(glm::vec3 pos, float radius, float damageAtCenter, float damageAtEdge);
		void UpdateExplosions(float timeDelta);
		void DoEnemyAvoidance(glm::vec3 playerPos, float timeDelta);
		void CollectActiveAndDespawning(glm::vec3 playerPos, float timeDelta);
		void UpdateEnemies(float timeDelta);
		void StartGame();
		void StopGame();

		struct ActiveMonster
		{
			glm::vec3 m_targetPosition;
			float m_radius;
			MonsterComponent* m_cc;
			Transform* m_transform;
		};
		bool m_enemiesEnabled = false;
		int m_avoidanceIterations = 1;				// increase for more stability but slower
		WorldGrid<uint32_t> m_activeMonsterGrid;	// indexes into vector below
		std::vector<ActiveMonster> m_activeMonsters;
		std::vector<EntityHandle> m_monstersToDespawn;
		int m_tileLoadRadius = 12;
		std::function<std::string(glm::ivec2)> m_worldTileSpawnFn;
	};
}