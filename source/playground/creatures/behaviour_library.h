#pragma once
#include "component_creature.h"

class EntitySystem;
class GraphicsSystem;

namespace BehaviourLibrary
{
	// These are basically factories for a bunch of useful behaviours
	// Its a lot faster to do anything heavy in c++, so this is a nice place for that

	// Moves from current position towards the creature move target at creature move speed
	// Sets the creature state when the target position is reached
	// Energy is modified by creature move cost
	Creature::Behaviour MoveToTarget(EntitySystem& es, GraphicsSystem& gs, Engine::Tag stateWhenTargetReached);

	// Transition to dying state if max age reached
	Creature::Behaviour DieAtMaxAge();

	// Transition to dying state if 0 energy reached
	Creature::Behaviour DieAtZeroEnergy();

	// Photosynthesize - increase energy a small amount each frame if in the sun
	Creature::Behaviour Photosynthesize();

	// Flee creatures containing flee tags
	Creature::Behaviour Flee(EntitySystem& es, GraphicsSystem& gs);
}