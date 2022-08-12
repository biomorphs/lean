#pragma once
#include "core/glm_headers.h"
#include <vector>
#include <robin_hood.h>

template<class PerTileData>
class WorldGrid
{
public:
	WorldGrid(glm::vec2 tileSize = { 32.0f,32.0f })
		: m_tileSize(tileSize)
	{
	}
	void Reset(bool removeOldTiles=false);
	void SetTileSize(glm::vec2 t);
	void AddEntry(glm::vec3 aabMin, glm::vec3 aabMax, const PerTileData& d);
	void FindEntries(glm::vec3 aabMin, glm::vec3 aabMax, std::vector<PerTileData>& results);
	void ForEachNearby(glm::vec3 aabMin, glm::vec3 aabMax, std::function<void(PerTileData&)> fn);
private:
	struct Tile
	{
		std::vector<PerTileData> m_entries;
	};

	glm::ivec2 PositionToTileIndex(glm::vec3 p) const;
	uint64_t TileIndexToHash(glm::ivec2) const;
	Tile& GetTile(glm::ivec2 t);
	Tile* GetTileMaybe(glm::ivec2 t);
	
	int m_initialTileEntries = 4;
	glm::vec2 m_tileSize = { 32.0f, 32.0f };
	robin_hood::unordered_map<uint64_t, std::unique_ptr<Tile>> m_tiles;
};

template<class PerTileData>
void WorldGrid<PerTileData>::SetTileSize(glm::vec2 t)
{
	if (t != m_tileSize)
	{
		Reset(true);
		m_tileSize = t;
	}
}

template<class PerTileData>
typename WorldGrid<PerTileData>::Tile& WorldGrid<PerTileData>::GetTile(glm::ivec2 t)
{
	const auto hash = TileIndexToHash(t);
	auto it = m_tiles.find(hash);
	if (it == m_tiles.end())
	{
		auto newTile = std::make_unique<Tile>();
		newTile->m_entries.reserve(m_initialTileEntries);
		auto newIt = m_tiles.insert({ hash,std::move(newTile) });
		return *(*(newIt.first)).second;	// argh my eyes!
	}
	return *(*it).second;
}

template<class PerTileData>
typename WorldGrid<PerTileData>::Tile* WorldGrid<PerTileData>::GetTileMaybe(glm::ivec2 t)
{
	const auto hash = TileIndexToHash(t);
	auto it = m_tiles.find(hash);
	if (it != m_tiles.end())
	{
		return (*it).second.get();
	}
	else
	{
		return nullptr;
	}
}

template<class PerTileData>
uint64_t WorldGrid<PerTileData>::TileIndexToHash(glm::ivec2 p) const
{
	return (uint64_t)p.x | ((uint64_t)p.y << 32);
}

template<class PerTileData>
glm::ivec2 WorldGrid<PerTileData>::PositionToTileIndex(glm::vec3 p) const
{
	const auto floored = glm::floor(p);
	return {floored.x / m_tileSize.x, floored.z / m_tileSize.y};
}

template<class PerTileData>
void WorldGrid<PerTileData>::Reset(bool removeOldTiles)
{
	if (removeOldTiles)
	{
		m_tiles.clear();
	}
	else
	{
		for (auto& it : m_tiles)
		{
			it.second->m_entries.clear();
		}
	}
}

template<class PerTileData>
void WorldGrid<PerTileData>::AddEntry(glm::vec3 aabMin, glm::vec3 aabMax, const PerTileData& d)
{
	auto tileIdMin = PositionToTileIndex(aabMin);
	auto tileIdMax = PositionToTileIndex(aabMax);
	if (tileIdMin == tileIdMax)
	{
		Tile& t = GetTile(tileIdMin);
		t.m_entries.emplace_back(d);
	}
	else
	{
		for (int tx = tileIdMin.x; tx <= tileIdMax.x; ++tx)
		{
			for (int tz = tileIdMin.y; tz <= tileIdMax.y; ++tz)
			{
				Tile& t = GetTile({ tx, tz });
				t.m_entries.emplace_back(d);
			}
		}
	}
}

template<class PerTileData>
void WorldGrid<PerTileData>::ForEachNearby(glm::vec3 aabMin, glm::vec3 aabMax, std::function<void(PerTileData&)> fn)
{
	auto tileIdMin = PositionToTileIndex(aabMin);
	auto tileIdMax = PositionToTileIndex(aabMax);
	if (tileIdMin == tileIdMax)
	{
		Tile* t = GetTileMaybe(tileIdMin);
		if (t != nullptr)
		{
			for (auto& entry : t->m_entries)
			{
				fn(entry);
			}
		}
	}
	else
	{
		static robin_hood::unordered_flat_set<PerTileData> uniqueData;
		uniqueData.clear();
		for (int tx = tileIdMin.x; tx <= tileIdMax.x; ++tx)
		{
			for (int tz = tileIdMin.y; tz <= tileIdMax.y; ++tz)
			{
				Tile* t = GetTileMaybe({ tx, tz });
				if (t != nullptr)
				{
					for (auto& entry : t->m_entries)
					{
						if (uniqueData.find(entry) == uniqueData.end())
						{
							uniqueData.insert(entry);
							fn(entry);
						}
					}
				}
			}
		}
	}
}

template<class PerTileData>
void WorldGrid<PerTileData>::FindEntries(glm::vec3 aabMin, glm::vec3 aabMax, std::vector<PerTileData>& results)
{
	auto tileIdMin = PositionToTileIndex(aabMin);
	auto tileIdMax = PositionToTileIndex(aabMax);
	if (tileIdMin == tileIdMax)
	{
		Tile& t = GetTile(tileIdMin);
		results.insert(results.end(), t.m_entries.begin(), t.m_entries.end());
	}
	else
	{
		for (int tx = tileIdMin.x; tx <= tileIdMax.x; ++tx)
		{
			for (int tz = tileIdMin.y; tz <= tileIdMax.y; ++tz)
			{
				Tile& t = GetTile({ tx, tz });
				results.insert(results.end(), t.m_entries.begin(), t.m_entries.end());
			}
		}
	}
}