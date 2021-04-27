#include "behaviour_library.h"
#include "component_creature.h"
#include "entity/entity_system.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_tags.h"

namespace BehaviourLibrary
{
	Creature::Behaviour Photosynthesize()
	{
		auto behaviour = [](EntityHandle e, Creature& c, float delta) {
			c.SetEnergy(c.GetEnergy() + 1.0f * delta);
			return true;
		};
		return behaviour;
	}

	Creature::Behaviour Flee(EntitySystem& es, GraphicsSystem& gs)
	{
		auto behaviour = [&es, &gs](EntityHandle e, Creature& c, float delta) {
			const auto world = es.GetWorld();
			for (const auto& visible : c.GetVisibleEntities())
			{
				const auto tags = world->GetComponent<Tags>(visible);
				if (tags)
				{
					bool shouldFlee = false;
					for (auto t : c.GetFleeFromTags())
					{
						if (tags->ContainsTag(t))
						{
							shouldFlee = true;
							break;
						}
					}
					if (shouldFlee)
					{
						auto mytransform = world->GetComponent<Transform>(e);
						auto p = mytransform->GetPosition();
						auto monstertransform = world->GetComponent<Transform>(visible);
						auto m = monstertransform->GetPosition();
						auto dir = glm::normalize(p - m);
						p = p + dir * 32.0f;
						//gs.DebugRenderer().DrawLine(p + glm::vec3(0.0f, 1.0f, 0.0f), m + glm::vec3(0.0f, 1.0f, 0.0f), { 1.0f,0.0f,0.0f });
						c.GetBlackboard()->SetVector("MoveToTarget", p);
						c.SetState("moveto");
						return false;
					}
				}
			}
			return true;
		};
		return behaviour;
	}

	Creature::Behaviour MoveToTarget(EntitySystem& es, GraphicsSystem& gs, Engine::Tag stateWhenTargetReached)
	{
		auto behaviour = [stateWhenTargetReached,&es,&gs](EntityHandle e, Creature& c, float delta) {
			const auto world = es.GetWorld();
			const auto transform = world->GetComponent<Transform>(e);
			const auto p = transform->GetPosition();
			const auto t = c.GetBlackboard()->GetVector("MoveToTarget");
			const float distanceToTarget = glm::length(t - p);
			const auto speed = c.GetMoveSpeed();
			if (distanceToTarget < speed * delta)
			{
				c.SetState(stateWhenTargetReached);
				return false;
			}
			const auto dir = glm::normalize(t - p);
			transform->SetPosition(p + dir * speed * delta);
			//gs.DebugRenderer().DrawLine(p + glm::vec3(0.0f, 1.0f, 0.0f), t + glm::vec3(0.0f, 1.0f, 0.0f), { 1.0f,1.0f,0.0f });
			c.SetEnergy(c.GetEnergy() - (c.GetMovementCost() * delta));
			return true;
		};
		return behaviour;
	}

	Creature::Behaviour DieAtMaxAge()
	{
		auto behaviour = [](EntityHandle e, Creature& c, float delta) {
			if (c.GetAge() >= c.GetMaxAge())
			{
				c.SetState("dying");
				return false;
			}
			return true;
		};
		return behaviour;
	}

	Creature::Behaviour DieAtZeroEnergy()
	{
		auto behaviour = [](EntityHandle e, Creature& c, float delta) {
			if (c.GetEnergy() <= 0.0f)
			{
				c.SetState("dying");
				return false;
			}
			return true;
		};
		return behaviour;
	}
}