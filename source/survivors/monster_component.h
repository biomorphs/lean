#pragma once

#include "entity/component.h"

class MonsterComponent
{
public:
	COMPONENT(MonsterComponent);
	COMPONENT_INSPECTOR();

	int GetCurrentHealth() { return m_currentHP; }
	float GetSpeed() { return m_speed; }
	void SetSpeed(float speed) { m_speed = speed; }

private:
	// active state
	int m_currentHP = 100;
	float m_speed = 2.0f;
};