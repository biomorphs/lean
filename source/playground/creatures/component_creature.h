#pragma once
#include "entity/component.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "engine/tag.h"
#include <set>
#include <robin_hood.h>

class Creature
{
public:
	COMPONENT(Creature);

	void SetMaxVisibleEntities(uint32_t m) { m_maxVisibleEntities = m; }
	uint32_t GetMaxVisibleEntities() { return m_maxVisibleEntities; }
	void SetAge(float a) { m_age = a; }
	float GetAge() { return m_age; }
	void SetMaxAge(float a) { m_maxAge = a; }
	float GetMaxAge() { return m_maxAge; }
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
	void AddFoodSourceTag(Engine::Tag t) { m_foodSourceTags.push_back(t); }
	const std::vector<Engine::Tag>& GetFoodSourceTags() const { return m_foodSourceTags; }
	
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
	// if a behaviour returns false, no other behaviours will be run for the current state
	using Behaviour = std::function<bool(EntityHandle, Creature&, float)>;
	using BehaviourSet = std::vector<Engine::Tag>;
	using StateBehaviours = robin_hood::unordered_map<Engine::Tag, BehaviourSet>;
	void AddBehaviour(Engine::Tag state, Engine::Tag behaviour);
	const StateBehaviours& GetBehaviours() const { return m_stateBehaviours; }

private:
	// params
	uint32_t m_maxVisibleEntities = 32;	// how many visible entities do we remember
	float m_wanderDistance = 10.0f;	// how far to wander when nothing else to do
	float m_hungerThreshold = 0.0f;	// when to get hungry
	float m_fullThreshold = 1.0f;	// how much to eat before 'full'
	float m_visionRadius = -1.0f;	// how far can I see? (<0 = no vision)
	float m_moveSpeed = 0.0f;		// top speed
	float m_movementCost = 5.0f;	// energy loss/second when moving
	float m_eatSpeed = 10.0f;		// energy gain/second when eating
	float m_maxAge = 100.0f;		// die after some time regardless of anything else
	std::vector<Engine::Tag> m_foodSourceTags;	// tags used to find edible creatures

	// state
	std::vector<EntityHandle> m_visibleEntities;	// what can I see?
	float m_currentEnergy = 0.0f;
	float m_age = 0.0f;
	glm::vec3 m_moveTarget = { 0.0f,0.0f,0.0f };
	EntityHandle m_foodTarget;
	Engine::Tag m_currentState = "idle";			// default state is always idle

	// behaviours
	StateBehaviours m_stateBehaviours;
};