#pragma once

#include "entity/component.h"

class PlayerComponent
{
public:
	COMPONENT(PlayerComponent);
	COMPONENT_INSPECTOR();

	int GetCurrentHealth() { return m_currentHP; }
	int GetMaximumHealth() { return m_maximumHP; }
	void SetMaximumHealth(int hp) { m_maximumHP = hp; }
	void SetCurrentHealth(int hp) { m_currentHP = hp; }
	int GetCurrentXP() { return m_currentXP; }
	void SetCurrentXP(int xp) { m_currentXP = xp; }
	int GetThisLevelXP() { return m_thisLevelXP; }
	void SetThisLevelXP(int xp) { m_thisLevelXP = xp; }
	int GetNextLevelXP() { return m_nextLevelXP; }
	void SetNextLevelXP(int xp) { m_nextLevelXP = xp; }
	float GetAreaMultiplier() { return m_multiArea; }
	void SetAreaMultiplier(float a) { m_multiArea = a; }
	float GetPickupAreaMultiplier() { return m_multiPickupArea; }
	void SetPickupAreaMultiplier(float a) { m_multiPickupArea = a; }
	float GetDamageMultiplier() { return m_multiDamage; }
	void SetDamageMultiplier(float a) { m_multiDamage = a; }
	float GetCooldownMultiplier() { return m_multiCooldown; }
	void SetCooldownMultiplier(float a) { m_multiCooldown = a; }
	float GetMoveSpeedMultiplier() { return m_multiMoveSpeed; }
	void SetMoveSpeedMultiplier(float a) { m_multiMoveSpeed = a; }
	void SetProjectileCount(int c) { m_projectileCount = c; }
	int GetProjectileCount() { return m_projectileCount; }

private:
	// active state
	int m_currentHP = 100;
	int m_maximumHP = 100;
	int m_currentXP = 0;
	int m_thisLevelXP = 0;
	int m_nextLevelXP = 100;

	// weapon stats
	int m_projectileCount = 1;
	float m_multiArea = 1.0f;
	float m_multiPickupArea = 1.0f;
	float m_multiDamage = 1.0f;
	float m_multiCooldown = 1.0f;
	float m_multiMoveSpeed = 1.0f;
};