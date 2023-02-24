#pragma once

#include "entity/component.h"
#include "entity/component_handle.h"
#include "engine/components/component_transform.h"

class AttractToEntityComponent
{
public:
	COMPONENT(AttractToEntityComponent);
	COMPONENT_INSPECTOR();

	void SetTarget(EntityHandle e) { m_target = e; }
	EntityHandle GetTargetEntity() { return m_target.GetEntity(); }
	Transform* GetTarget() { return m_target.GetComponent(); }
	void SetTargetOffset(glm::vec3 o) { m_targetOffset = o; }
	glm::vec3 GetTargetOffset() { return m_targetOffset; }
	void SetAcceleration(float r) { m_acceleration = r; }
	float GetAcceleration() { return m_acceleration; }
	void SetMaxSpeed(float r) { m_maxSpeed = r; }
	float GetMaxSpeed() { return m_maxSpeed; }
	void SetTriggerRange(float r) { m_triggerRange = r; }
	float GetTriggerRange() { return m_triggerRange; }
	void SetActivationRange(float r) { m_activationRange = r; }
	float GetActivationRange() { return m_activationRange; }
	using TriggerFn = std::function<void(EntityHandle)>;
	void SetTriggerCallback(TriggerFn fn) { m_triggerFn = fn; }
	const TriggerFn& GetTriggerCallback() {	return m_triggerFn;	}
	glm::vec3 GetVelocity() { return m_velocity; }
	void SetVelocity(glm::vec3 v) { m_velocity = v; }

private:
	ComponentHandle<Transform> m_target;
	glm::vec3 m_targetOffset = { 0,0,0 };	// offset local to target entity
	float m_acceleration = 1.0f;	// rate of attraction speed increase	
	float m_maxSpeed = 1.0f;	
	float m_triggerRange = 1.0f;	// trigger callbacks if this close
	float m_activationRange = 64.0f;	// dont attract outside this radius
	TriggerFn m_triggerFn;	// called when within trigger range
	glm::vec3 m_velocity = { 0,0,0 };
};