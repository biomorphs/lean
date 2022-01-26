#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"

class EnvironmentSettings
{
public:
	COMPONENT(EnvironmentSettings);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui);

	inline void SetClearColour(glm::vec3 c) { m_clearColour = c; }
	inline glm::vec3 GetClearColour() const { return m_clearColour; }

	inline void SetGravity(glm::vec3 g) { m_gravity = g; }
	inline glm::vec3 GetGravity() const { return m_gravity; }

private:
	glm::vec3 m_clearColour;
	glm::vec3 m_gravity = { 0.0f, -9.81f, 0.0f };
};