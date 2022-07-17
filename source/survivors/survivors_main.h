#pragma once
#include "engine/system.h"
#include "core/glm_headers.h"
#include <functional>
#include <string>

namespace Survivors
{
	class SurvivorsMain : public Engine::System
	{
	public:
		virtual bool PostInit();
		virtual bool Tick(float timeDelta);
		virtual void Shutdown();

	private:
		void StartGame();
		void StopGame();
		int m_tileLoadRadius = 7;
		std::function<std::string(glm::ivec2)> m_worldTileSpawnFn;
		bool m_enemiesEnabled = false;
	};
}