#pragma once
#include <stdint.h>
#include "engine/serialisation.h"

namespace Engine
{
	class ScriptSystem;
}

class EntityHandle
{
public:
	EntityHandle() = default;
	EntityHandle(uint32_t id) : m_id(id) {}
	uint32_t GetID() const { return m_id; }
	bool IsValid() { return m_id != -1; }
	bool operator==(const EntityHandle& other) const { return m_id == other.m_id; }
	bool operator<(const EntityHandle& other) const { return m_id < other.m_id; }
	
	// We need to remap entity IDs during loading sometimes
	// Not thread safe!
	using OnLoaded = std::function<void(EntityHandle&)>;
	static void SetOnLoadFinishCallback(OnLoaded fn) { m_onLoadedFn = fn; }
	SERIALISED_CLASS();
private:
	uint32_t m_id = -1;
	static OnLoaded m_onLoadedFn;
};