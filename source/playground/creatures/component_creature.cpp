#include "component_creature.h"

COMPONENT_SCRIPTS(Creature, 
	"SetAge", &Creature::SetAge,
	"GetAge", &Creature::GetAge,
	"SetMaxAge", &Creature::SetMaxAge,
	"GetMaxAge", &Creature::GetMaxAge,
	"SetMoveSpeed", &Creature::SetMoveSpeed,
	"GetMoveSpeed", &Creature::GetMoveSpeed,
	"SetVisionRadius", &Creature::SetVisionRadius,
	"GetVisionRadius", &Creature::GetVisionRadius,
	"GetVisibleEntities", &Creature::GetVisibleEntities,
	"SetHungerThreshold", &Creature::SetHungerThreshold,
	"GetHungerThreshold", &Creature::GetHungerThreshold,
	"SetEnergy", &Creature::SetEnergy,
	"GetEnergy", &Creature::GetEnergy,
	"AddBehaviour", &Creature::AddBehaviour,
	"SetState", &Creature::SetState,
	"GetState", &Creature::GetState,
	"GetMovementCost", &Creature::GetMovementCost,
	"SetMovementCost", &Creature::SetMovementCost,
	"SetEatingSpeed", &Creature::SetEatingSpeed,
	"GetEatingSpeed", &Creature::GetEatingSpeed,
	"SetFullThreshold", &Creature::SetFullThreshold,
	"GetFullThreshold", &Creature::GetFullThreshold,
	"SetWanderDistance", &Creature::SetWanderDistance,
	"GetWanderDistance", &Creature::GetWanderDistance,
	"AddFoodSourceTag", &Creature::AddFoodSourceTag,
	"GetFoodSourceTags", &Creature::GetFoodSourceTags,
	"AddVisionTag", &Creature::AddVisionTag,
	"GetVisionTags", &Creature::GetVisionTags,
	"GetMaxVisibleEntities", &Creature::GetMaxVisibleEntities,
	"SetMaxVisibleEntities", &Creature::SetMaxVisibleEntities,
	"AddFleeFromTag", &Creature::AddFleeFromTag,
	"GetFleeFromTags", &Creature::GetFleeFromTags,
	"GetBlackboard", &Creature::GetBlackboard
);

void Creature::AddBehaviour(Engine::Tag state, Engine::Tag behaviour)
{
	auto& behaviours = m_stateBehaviours[state];
	behaviours.push_back(behaviour);
}
