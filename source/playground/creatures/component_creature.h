#pragma once
#include "entity/component.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "engine/tag.h"
#include <robin_hood.h>

class Creature
{
public:
	COMPONENT(Creature);

	void SetWanderDistance(float r) { m_wanderDistance = r; }
	float GetWanderDistance() { return m_wanderDistance; }
	void SetVisionRadius(float r) { m_visionRadius = r; }
	float GetVisionRadius() { return m_visionRadius; }
	void SetHungerThreshold(float t) { m_hungerThreshold = t; }
	float GetHungerThreshold() { return m_hungerThreshold; }
	void SetMoveSpeed(float s) { m_moveSpeed = s; }
	float GetMoveSpeed() { return m_moveSpeed; }
	void SetMovementCost(float c) { m_movementCost = c; }
	float GetMovementCost() { return m_movementCost; }
	void SetEatingSpeed(float s) { m_eatSpeed = s; }
	float GetEatingSpeed() { return m_eatSpeed; }
	void SetFullThreshold(float s) { m_fullThreshold = s; }
	float GetFullThreshold() { return m_fullThreshold; }
	
	void SetFoodTarget(EntityHandle e) { m_foodTarget = e; }
	EntityHandle GetFoodTarget() { return m_foodTarget; }
	void SetMoveTarget(glm::vec3 t) { m_moveTarget = t; }
	glm::vec3 GetMoveTarget() { return m_moveTarget; }
	void SetEnergy(float e) { m_currentEnergy = e; }
	float GetEnergy() { return m_currentEnergy; }
	void SetVisibleEntities(std::vector<EntityHandle>&& e) { m_visibleEntities = std::move(e); }
	const std::vector<EntityHandle>& GetVisibleEntities() const { return m_visibleEntities; }
	void SetState(Engine::Tag state) { m_currentState = state; }
	Engine::Tag GetState() { return m_currentState; }
	
	// states and behaviours are simply tags
	// behaviours are tagged functions associated with a state
	using BehaviourSet = std::vector<Engine::Tag>;
	using StateBehaviours = robin_hood::unordered_map<Engine::Tag, BehaviourSet>;
	void AddBehaviour(Engine::Tag state, Engine::Tag behaviour);
	const StateBehaviours& GetBehaviours() const { return m_stateBehaviours; }

private:
	// params
	float m_wanderDistance = 10.0f;	// how far to wander when nothing else to do
	float m_hungerThreshold = 0.0f;	// when to get hungry
	float m_fullThreshold = 1.0f;	// how much to eat before 'full'
	float m_visionRadius = -1.0f;	// how far can I see? (<0 = no vision)
	float m_moveSpeed = 0.0f;		// top speed
	float m_movementCost = 5.0f;	// energy loss/second when moving
	float m_eatSpeed = 10.0f;		// energy gain/second when eating

	// state
	std::vector<EntityHandle> m_visibleEntities;	// what can I see?
	float m_currentEnergy = 0.0f;
	glm::vec3 m_moveTarget = { 0.0f,0.0f,0.0f };
	EntityHandle m_foodTarget;
	Engine::Tag m_currentState = "<unknown>";

	// behaviours
	StateBehaviours m_stateBehaviours;
};