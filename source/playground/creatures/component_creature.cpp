#include "component_creature.h"

COMPONENT_SCRIPTS(Creature, 
	"SetAge", &Creature::SetAge,
	"GetAge", &Creature::GetAge,
	"SetMaxAge", &Creature::SetMaxAge,
	"GetMaxAge", &Creature::GetMaxAge,
	"SetMoveTarget", &Creature::SetMoveTarget,
	"GetMoveTarget", &Creature::GetMoveTarget,
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
	"GetMovementCost", &Creature::GetMovementCost,
	"SetMovementCost", &Creature::SetMovementCost,
	"SetEatingSpeed", &Creature::SetEatingSpeed,
	"GetEatingSpeed", &Creature::GetEatingSpeed,
	"SetFullThreshold", &Creature::SetFullThreshold,
	"GetFullThreshold", &Creature::GetFullThreshold,
	"SetFoodTarget", &Creature::SetFoodTarget,
	"GetFoodTarget", &Creature::GetFoodTarget,
	"SetWanderDistance", &Creature::SetWanderDistance,
	"GetWanderDistance", &Creature::GetWanderDistance,
	"AddFoodSourceTag", &Creature::AddFoodSourceTag,
	"GetFoodSourceTags", &Creature::GetFoodSourceTags
);

void Creature::AddBehaviour(Engine::Tag state, Engine::Tag behaviour)
{
	auto& behaviours = m_stateBehaviours[state];
	behaviours.push_back(behaviour);
}
