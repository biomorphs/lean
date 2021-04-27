#pragma once
#include "engine/tag.h"
#include "entity/entity_handle.h"
#include <robin_hood.h>

// Getting a value that isn't set is an error!
class Blackboard
{
public:
	using Ints = robin_hood::unordered_map<Engine::Tag, int>;
	inline bool ContainsInt(Engine::Tag t) { return m_ints.contains(t); }
	inline void RemoveInt(Engine::Tag t) { m_ints.erase(t); }
	inline void SetInt(Engine::Tag tag, int v) { m_ints[tag] = v; }
	inline int GetInt(Engine::Tag tag) { return m_ints[tag]; }
	inline const Ints& GetInts() const { return m_ints; }

	using Entities = robin_hood::unordered_map<Engine::Tag, EntityHandle>;
	inline bool ContainsEntity(Engine::Tag t) { return m_entities.contains(t); }
	inline void RemoveEntity(Engine::Tag t) { m_entities.erase(t); }
	inline void SetEntity(Engine::Tag tag, EntityHandle v) { m_entities[tag] = v; }
	inline EntityHandle GetEntity(Engine::Tag tag) { return m_entities[tag]; }
	inline const Entities& GetEntities() const { return m_entities; }

	using Vectors = robin_hood::unordered_map<Engine::Tag, glm::vec3>;
	inline bool ContainsVector(Engine::Tag t) { return m_vectors.contains(t); }
	inline void RemoveVector(Engine::Tag t) { m_vectors.erase(t); }
	inline void SetVector3(Engine::Tag tag, float x, float y, float z) { m_vectors[tag] = { x,y,z }; }
	inline void SetVector(Engine::Tag tag, glm::vec3 v) { m_vectors[tag] = v; }
	inline glm::vec3 GetVector(Engine::Tag tag) { return m_vectors[tag]; }
	inline const Vectors& GetVectors() const { return m_vectors; }
private:
	Ints m_ints;
	Entities m_entities;
	Vectors m_vectors;
};