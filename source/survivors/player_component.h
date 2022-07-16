#pragma once

#include "entity/component.h"

class PlayerComponent
{
public:
	COMPONENT(PlayerComponent);
	COMPONENT_INSPECTOR();

	int GetCurrentHealth() { return m_currentHP; }
	float GetCurrentSpeed() { return m_currentSpeed; }
	float GetCurrentDirection() { return m_currentDirection; }

private:
	// active state
	int m_currentHP = 100;
	float m_currentSpeed = 0.0f;
	float m_currentDirection = 0.0f;
};