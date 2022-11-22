#pragma once

#include "entity/component.h"
#include "entity/component_handle.h"
#include "engine/components/component_model_part_materials.h"

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
	float GetRagdollChance() { return m_ragdollChance; }
	void SetRagdollChance(float c) { m_ragdollChance = c; }
	void SetDamagedMaterialEntity(EntityHandle e) { m_damagedMaterial = e; }
	EntityHandle GetDamagedMaterialEntity() { return m_damagedMaterial.GetEntity(); }
	void SetLastDamagedTime(double t) { m_lastDamagedTime = t; }
	double GetLastDamagedTime() { return m_lastDamagedTime; }
	void SetLastAttackedTime(double t) { m_lastAttackTime = t; }
	double GetLastAttackedTime() { return m_lastAttackTime; }
	void SetAttackFrequency(double t) { m_attackFrequency = t; }
	double GetAttackFrequency() { return m_attackFrequency; }
	void SetAttackMinValue(int v) { m_damageMinValue = v; }
	int GetAttackMinValue() { return m_damageMinValue; }
	void SetAttackMaxValue(int v) { m_damageMaxValue = v; }
	int GetAttackMaxValue() { return m_damageMaxValue; }
	int GetXPOnDeath() { return m_xpGranted; }
	void SetXPOnDeath(int g) { m_xpGranted = g; };

private:
	// active state
	double m_lastDamagedTime = -1;
	double m_lastAttackTime = -1;
	float m_currentHP = 100;
	glm::vec2 m_knockBack = { 0.0f,0.0f };

	// parameters
	float m_ragdollChance = 0.1f;
	float m_speed = 4.0f;
	float m_visionRadius = 450.0f;
	float m_despawnRadius = 500.0f;
	float m_collideRadius = 2.0f;
	float m_knockBackFalloff = 0.95f;
	double m_attackFrequency = 1;
	int m_damageMinValue = 5;
	int m_damageMaxValue = 10;
	int m_xpGranted = 5;
	ComponentHandle<ModelPartMaterials> m_damagedMaterial;
};