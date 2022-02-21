#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"

class WalkableArea
{
public:
	COMPONENT(WalkableArea);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui);

	glm::vec3 GetBoundsMin() const { return m_boundsMin; }
	glm::vec3 GetBoundsMax() const { return m_boundsMax; }
	void SetBounds(glm::vec3 bMin, glm::vec3 bMax);
	glm::ivec2 GetGridResolution() const { return m_gridResolution; }
	void SetGridResolution(glm::ivec2 r);
	void SetGridValue(glm::ivec2 pos, float value);
	float GetGridValue(glm::ivec2 pos);

private:
	glm::vec3 m_boundsMin = { 0.0f,0.0f,0.0f };
	glm::vec3 m_boundsMax = { 1.0f,1.0f,1.0f };
	glm::ivec2 m_gridResolution = {0, 0};
	std::vector<float> m_grid;
};