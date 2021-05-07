#pragma once
#include "engine/tag.h"
#include "entity/entity_handle.h"
#include <robin_hood.h>

// Getting a value that isn't set is an error!
class Blackboard
{
public:
	using Ints = robin_hood::unordered_map<Engine::Tag, int>;
	inline bool ContainsInt(const Engine::Tag& t) { return m_ints.contains(t); }
	inline void RemoveInt(const Engine::Tag& t) { m_ints.erase(t); }
	inline void SetInt(const Engine::Tag& tag, int v) { m_ints[tag] = v; }
	inline int GetInt(const Engine::Tag& tag) { return m_ints[tag]; }
	inline const Ints& GetInts() const { return m_ints; }

	using Floats = robin_hood::unordered_map<Engine::Tag, float>;
	inline bool ContainsFloat(const Engine::Tag& t) { return m_floats.contains(t); }
	inline void RemoveFloat(const Engine::Tag& t) { m_floats.erase(t); }
	inline void SetFloat(const Engine::Tag& tag, float v) { m_floats[tag] = v; }
	inline float GetFloat(const Engine::Tag& tag) { return m_floats[tag]; }
	inline const Floats& GetFloats() const { return m_floats; }

	using Entities = robin_hood::unordered_map<Engine::Tag, EntityHandle>;
	inline bool ContainsEntity(const Engine::Tag& t) { return m_entities.contains(t); }
	inline void RemoveEntity(const Engine::Tag& t) { m_entities.erase(t); }
	inline void SetEntity(const Engine::Tag& tag, EntityHandle v) { m_entities[tag] = v; }
	inline EntityHandle GetEntity(const Engine::Tag& tag) { return m_entities[tag]; }
	inline const Entities& GetEntities() const { return m_entities; }

	using Vectors = robin_hood::unordered_map<Engine::Tag, glm::vec3>;
	inline bool ContainsVector(const Engine::Tag& t) { return m_vectors.contains(t); }
	inline void RemoveVector(const Engine::Tag& t) { m_vectors.erase(t); }
	inline void SetVector3(const Engine::Tag& tag, float x, float y, float z) { m_vectors[tag] = { x,y,z }; }
	inline void SetVector(const Engine::Tag& tag, glm::vec3 v) { m_vectors[tag] = v; }
	inline glm::vec3 GetVector(const Engine::Tag& tag) { return m_vectors[tag]; }
	inline const Vectors& GetVectors() const { return m_vectors; }
private:
	Ints m_ints;
	Floats m_floats;
	Entities m_entities;
	Vectors m_vectors;
};