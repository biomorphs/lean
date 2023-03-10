#pragma once

#include "entity/component.h"
#include "entity/entity_handle.h"

class ProjectileComponent
{
public:
	COMPONENT(ProjectileComponent);
	void SetMaxDistance(float v) { m_maxDistance = v; }
	float GetMaxDistance() { return m_maxDistance; }
	void SetVelocity(glm::vec3 v) { m_velocity = v; }
	glm::vec3 GetVelocity() { return m_velocity; }
	void SetDamageRange(float mind, float maxd) { m_maxHitDamage = maxd; m_minHitDamage = mind; }
	float GetMinDamage() { return m_minHitDamage; }
	float GetMaxDamage() { return m_maxHitDamage; }
	void SetRadius(float radius) { m_radius = radius; }
	float GetRadius() { return m_radius; }
	void AddDistanceTravelled(float v) { m_distanceTravelled += v; }
	float GetDistanceTravelled() { return m_distanceTravelled; }
	const std::vector<EntityHandle>& GetHitObjects() const { return m_hitObjects; }
	std::vector<EntityHandle>& GetHitObjects() { return m_hitObjects; }
private:
	// params
	float m_maxDistance = 256.0f;
	glm::vec3 m_velocity = { 0.0f, 0.0f, 1.0f };
	int m_minHitDamage = 1;
	int m_maxHitDamage = 10;
	float m_radius = 0.25f;	// ^^

	// active state
	float m_distanceTravelled = 0.0f;
	std::vector<EntityHandle> m_hitObjects;		// keeps track of anything already hit
};