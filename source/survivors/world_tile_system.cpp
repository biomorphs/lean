#include "world_tile_system.h"
#include "world_tile_component.h"
#include "engine/system_manager.h"
#include "engine/script_system.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_tags.h"
#include "core/file_io.h"

namespace Survivors
{
	uint64_t WorldTileSystem::TilePositionHash(glm::ivec2 p)
	{
		return ((uint64_t)p.x & 0x00000000ffffffff) | ((uint64_t)p.y << 32);
	}

	glm::ivec2 WorldTileSystem::PositionToTile(glm::vec3 p)
	{ 
		return glm::ivec2(p.x / m_tileSize, p.z / m_tileSize);
	}

	bool WorldTileSystem::PostInit()
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		entities->RegisterComponentType<WorldTileComponent>();
		entities->RegisterInspector<WorldTileComponent>(WorldTileComponent::MakeInspector());

		auto scripts = Engine::GetSystem<Engine::ScriptSystem>("Script");
		auto survivors = scripts->Globals()["Survivors"].get_or_create<sol::table>();
		survivors["SpawnTile"] = [this](const std::string& tileSrcPath, glm::ivec2 tileOrigin) {
			return SpawnTile(tileSrcPath, tileOrigin);
		};
		survivors["DestroyTileAt"] = [this](glm::ivec2 target) {
			DestroyTileAt(target);
		};

		return true;
	}

	void WorldTileSystem::Shutdown()
	{
		m_loadedTileData.clear();
	}

	void WorldTileSystem::DestroyAll()
	{
		while (m_activeTiles.size() > 0)
		{
			DestroyTileAt(m_activeTiles.begin()->second.m_origin);
		}
	}

	void WorldTileSystem::EnsureLoadedExclusive(glm::ivec2 tileMin, glm::ivec2 tileMax, SpawnTileFn spawnFn)
	{
		SDE_PROF_EVENT();

		std::vector<glm::ivec2> tilesToRemove;
		for (auto& it : m_activeTiles)
		{
			if (glm::any(glm::lessThan(it.second.m_origin, tileMin)) ||
				glm::any(glm::greaterThan(it.second.m_origin, tileMax)))
			{
				tilesToRemove.push_back(it.second.m_origin);
			}
		}
		for (auto it : tilesToRemove)
		{
			DestroyTileAt(it);
		}

		for (int ix = tileMin.x; ix < tileMax.x; ++ix)
		{
			for (int iz = tileMin.y; iz < tileMax.y; ++iz)
			{
				auto tilePos = glm::ivec2(ix, iz);
				if (m_activeTiles.find(TilePositionHash(tilePos)) == m_activeTiles.end())
				{
					std::string spawnPath = spawnFn(tilePos);
					if (spawnPath != "")
					{
						SpawnTile(spawnPath, tilePos);
					}
				}
			}
		}
	}

	EntityHandle WorldTileSystem::SpawnTile(const std::string& tileSrcPath, glm::ivec2 tileOrigin)
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		World* world = entities->GetWorld();

		// if there is already a tile then return the existing one
		uint64_t tileHash = TilePositionHash(tileOrigin);
		auto foundTile = m_activeTiles.find(tileHash);
		if (foundTile != m_activeTiles.end())
		{
			return foundTile->second.m_tileEntity;
		}

		// load the json if we need to
		auto foundIt = m_loadedTileData.find(tileSrcPath);
		if(foundIt == m_loadedTileData.end())
		{
			SDE_PROF_EVENT("LoadAndParseJson");

			std::string sceneText;
			if (!Core::LoadTextFromFile(tileSrcPath, sceneText))
			{
				return -1;
			}
			m_loadedTileData[tileSrcPath] = std::make_unique<nlohmann::json>();
			foundIt = m_loadedTileData.find(tileSrcPath);
			*foundIt->second = nlohmann::json::parse(sceneText);
		}
		assert(foundIt != m_loadedTileData.end());

		// create the tile entity
		EntityHandle newTileEntity = world->AddEntity();
		m_activeTiles[tileHash] = { tileOrigin, newTileEntity };
		world->AddComponent(newTileEntity, WorldTileComponent::GetType());
		WorldTileComponent* worldTile = world->GetComponent<WorldTileComponent>(newTileEntity);
		world->AddComponent(newTileEntity, Transform::GetType());
		Transform* tileTransform = world->GetComponent<Transform>(newTileEntity);
		tileTransform->SetPosition({ tileOrigin.x * m_tileSize, 0.0f, tileOrigin.y * m_tileSize });
		world->AddComponent(newTileEntity, Tags::GetType());
		Tags* tileTags = world->GetComponent<Tags>(newTileEntity);
		tileTags->AddTag("Survivors::WorldTile");
		tileTags->AddTag(tileSrcPath.c_str());

		// serialise the tile entities
		glm::mat4 tileTransformMatrix = glm::translate(tileTransform->GetPosition());
		std::vector<uint32_t> newEntities = entities->SerialiseEntities(*foundIt->second, false);
		for (const auto id : newEntities)
		{
			worldTile->OwnedChildren().emplace_back(id);

			// transform all children with no parent relative to the tile
			// we dont use parent transform since it messes with physx
			auto transform = world->GetComponent<Transform>(id);
			if (transform)
			{
				if (transform->GetParent().GetEntity().GetID() == -1)
				{
					auto newMat = tileTransformMatrix * transform->GetMatrix();
					auto newPos = glm::vec3(newMat[3]);
					transform->SetPosition(newPos);
				}
			}
		}

		return newTileEntity;
	}

	void WorldTileSystem::DestroyTileAt(glm::ivec2 tileOrigin)
	{
		SDE_PROF_EVENT();

		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		World* world = entities->GetWorld();

		uint64_t tileHash = TilePositionHash(tileOrigin);
		auto foundTile = m_activeTiles.find(tileHash);
		if (foundTile != m_activeTiles.end())
		{
			WorldTileComponent* worldTile = world->GetComponent<WorldTileComponent>(foundTile->second.m_tileEntity);
			if (worldTile)
			{
				for (auto it : worldTile->OwnedChildren())
				{
					world->RemoveEntity(it);
				}
			}
			world->RemoveEntity(foundTile->second.m_tileEntity);
			m_activeTiles.erase(foundTile);
		}
	}
}