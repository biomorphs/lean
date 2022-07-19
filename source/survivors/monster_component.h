#pragma once

#include "entity/component.h"

class MonsterComponent
{
public:
	COMPONENT(MonsterComponent);
	COMPONENT_INSPECTOR();

	void SetCurrentHealth(float hp) { m_currentHP = hp; }
	float GetCurrentHealth() { return m_currentHP; }
	float GetSpeed() { return m_speed; }
	void SetSpeed(float speed) { m_speed = speed; }
	float GetVisionRadius() { return m_visionRadius; }
	void SetVisionRadius(float f) { m_visionRadius = f; }
	float GetDespawnRadius() { return m_despawnRadius; }
	void SetDespawnRadius(float f) { m_despawnRadius = f; }
	float GetCollideRadius() { return m_collideRadius; }
	void SetCollideRadius(float r) { m_collideRadius = r; }
	void AddKnockback(glm::vec2 v) { m_knockBack += v; }
	void SetKnockback(glm::vec2 v) { m_knockBack = v; }
	glm::vec2 GetKnockback() { return m_knockBack; }
	void SetKnockbackFalloff(float v) { m_knockBackFalloff = v; }
	float GetKnockbackFalloff() { return m_knockBackFalloff; }

private:
	// active state
	float m_currentHP = 100;
	float m_speed = 4.0f;
	float m_visionRadius = 450.0f;
	float m_despawnRadius = 500.0f;
	float m_collideRadius = 2.0f;
	glm::vec2 m_knockBack = { 0.0f,0.0f };
	float m_knockBackFalloff = 0.95f;
};