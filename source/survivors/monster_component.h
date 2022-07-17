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
	float GetVisionRadius() { return m_visionRadius; }
	void SetVisionRadius(float f) { m_visionRadius = f; }
	float GetDespawnRadius() { return m_despawnRadius; }
	void SetDespawnRadius(float f) { m_despawnRadius = f; }

private:
	// active state
	int m_currentHP = 100;
	float m_speed = 4.0f;
	float m_visionRadius = 400.0f;
	float m_despawnRadius = 800.0f;
};