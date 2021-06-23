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
	SERIALISED_CLASS();
private:
	uint32_t m_id = -1;
};