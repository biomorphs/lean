#pragma once
#include "engine/tag.h"
#include "entity/entity_handle.h"
#include <robin_hood.h>

namespace Behaviours
{
	// Getting a value that isn't set is an error!
	class Blackboard
	{
	public:
		void Reset();

		bool IsKey(std::string_view s) { return s.size() > 0 && s.data()[0] == '@'; }

		using Ints = robin_hood::unordered_map<Engine::Tag, int>;
		inline bool ContainsInt(const Engine::Tag& t) { return m_ints.contains(t); }
		inline void RemoveInt(const Engine::Tag& t) { m_ints.erase(t); }
		inline void SetInt(const Engine::Tag& tag, int v) { m_ints[tag] = v; }
		inline int GetInt(const Engine::Tag& tag) { return m_ints[tag]; }
		inline int TryGetInt(const Engine::Tag& tag, int defaultVal=-1) { auto f = m_ints.find(tag); if (f != m_ints.end())	return f->second; else return defaultVal; }
		inline Ints& GetInts() { return m_ints; }

		using Floats = robin_hood::unordered_map<Engine::Tag, float>;
		inline bool ContainsFloat(const Engine::Tag& t) { return m_floats.contains(t); }
		inline void RemoveFloat(const Engine::Tag& t) { m_floats.erase(t); }
		inline void SetFloat(const Engine::Tag& tag, float v) { m_floats[tag] = v; }
		inline float GetFloat(const Engine::Tag& tag) { return m_floats[tag]; }
		inline float TryGetFloat(const Engine::Tag& tag, float defaultVal = 0.0f) { auto f = m_floats.find(tag); if (f != m_floats.end()) return f->second; else return defaultVal; }
		inline Floats& GetFloats() { return m_floats; }
		float GetOrParseFloat(const Engine::Tag& tag);

		using Entities = robin_hood::unordered_map<Engine::Tag, EntityHandle>;
		inline bool ContainsEntity(const Engine::Tag& t) { return m_entities.contains(t); }
		inline void RemoveEntity(const Engine::Tag& t) { m_entities.erase(t); }
		inline void SetEntity(const Engine::Tag& tag, EntityHandle v) { m_entities[tag] = v; }
		inline EntityHandle GetEntity(const Engine::Tag& tag) { return m_entities[tag]; }
		inline EntityHandle TryGetEntity(const Engine::Tag& tag, EntityHandle defaultVal = EntityHandle(-1)) { auto f = m_entities.find(tag); if (f != m_entities.end()) return f->second; else return defaultVal; }
		inline Entities& GetEntities() { return m_entities; }

		using Vectors = robin_hood::unordered_map<Engine::Tag, glm::vec3>;
		inline bool ContainsVector(const Engine::Tag& t) { return m_vectors.contains(t); }
		inline void RemoveVector(const Engine::Tag& t) { m_vectors.erase(t); }
		inline void SetVector3(const Engine::Tag& tag, float x, float y, float z) { m_vectors[tag] = { x,y,z }; }
		inline void SetVector(const Engine::Tag& tag, glm::vec3 v) { m_vectors[tag] = v; }
		inline glm::vec3 GetVector(const Engine::Tag& tag) { return m_vectors[tag]; }
		inline glm::vec3 TryGetVector(const Engine::Tag& tag, glm::vec3 defaultVal = glm::vec3(0.0f)) { auto f = m_vectors.find(tag); if (f != m_vectors.end())	return f->second; else return defaultVal; }
		inline Vectors& GetVectors() { return m_vectors; }
	private:
		Ints m_ints;
		Floats m_floats;
		Entities m_entities;
		Vectors m_vectors;
	};
}