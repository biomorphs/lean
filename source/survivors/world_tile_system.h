#pragma once
#include "engine/system.h"
#include "engine/serialisation.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace Survivors
{
	class WorldTileSystem : public Engine::System
	{
	public:
		using SpawnTileFn = std::function<std::string(glm::ivec2)>;
		void EnsureLoadedExclusive(glm::ivec2 tileMin, glm::ivec2 tileMax, SpawnTileFn spawnFn);

		EntityHandle SpawnTile(const std::string& tileSrcPath, glm::ivec2 tileOrigin);
		void DestroyTileAt(glm::ivec2 tileOrigin);
		void DestroyAll();

		glm::ivec2 PositionToTile(glm::vec3 p);

		virtual bool PostInit();
		virtual void Shutdown();

	private:
		struct ActiveTileRecord
		{
			glm::ivec2 m_origin;
			EntityHandle m_tileEntity;
		};
		uint64_t TilePositionHash(glm::ivec2 p);
		std::unordered_map<uint64_t, ActiveTileRecord> m_activeTiles;	// key = TilePositionHash
		std::unordered_map<std::string, std::unique_ptr<nlohmann::json>> m_loadedTileData;
		const float m_tileSize = 64.0f;

	};
}