#pragma once
#include "engine/system.h"
#include "core/glm_headers.h"
#include "entity_grid.h"
#include <functional>
#include <string>

class MonsterComponent;
class Transform;
namespace Survivors
{
	class SurvivorsMain : public Engine::System
	{
	public:
		virtual bool PostInit();
		virtual bool Tick(float timeDelta);
		virtual void Shutdown();

	private:
		void UpdateEnemies();
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
		WorldGrid<uint32_t> m_activeMonsterGrid;	// indexes into vector below
		std::vector<ActiveMonster> m_activeMonsters;
		int m_tileLoadRadius = 12;
		std::function<std::string(glm::ivec2)> m_worldTileSpawnFn;
	};
}