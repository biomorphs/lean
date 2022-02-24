#include "character_controller_system.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "core/profiler.h"
#include "entity/entity_system.h"
#include "entity/world.h"
#include "engine/physics_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_character_controller.h"
#include "components/component_environment_settings.h"
#include "engine/physics_system.h"

namespace Engine
{
	static float s_overlapRecoverPerFrame = 0.1f;

	CharacterControllerSystem::CharacterControllerSystem()
	{
	}

	CharacterControllerSystem::~CharacterControllerSystem()
	{
	}

	void CharacterControllerSystem::RegisterComponents()
	{
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		entities->RegisterComponentType<CharacterController>();
		entities->RegisterInspector<CharacterController>(CharacterController::MakeInspector());
	}

	void CharacterControllerSystem::RegisterScripts()
	{
	}

	bool CharacterControllerSystem::PreInit()
	{
		SDE_PROF_EVENT();
		return true;
	}

	bool CharacterControllerSystem::Initialise()
	{
		SDE_PROF_EVENT();

		RegisterComponents();
		RegisterScripts();

		return true;
	}

	bool CharacterControllerSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto physics = Engine::GetSystem<PhysicsSystem>("Physics");
		auto world = entities->GetWorld();
		
		// get global gravity value from world
		glm::vec3 gravity(0.0f, -9.8f, 0.0f);
		world->ForEachComponent<EnvironmentSettings>([&gravity](EnvironmentSettings& s, EntityHandle owner) {
			gravity = s.GetGravity();
		});
		
		static auto s_it = world->MakeIterator<CharacterController, Transform>();
		s_it.ForEach([graphics, physics, timeDelta, gravity](CharacterController& c, Transform& t, EntityHandle e) {
			glm::mat4 matrix = glm::translate(glm::identity<glm::mat4>(), t.GetPosition() + glm::vec3(0.0f,c.GetVerticalOffset(),0.0f));
			matrix = matrix * glm::toMat4(t.GetOrientation());
			glm::vec3 worldSpacePos = glm::vec3(matrix[3]);
			graphics->DebugRenderer().DrawCapsule(c.GetRadius(), c.GetHalfHeight(), { 1.0f,0.0f,0.0f,1.0f }, matrix);

			glm::vec3 newPosition = t.GetPosition();
			glm::vec3 currentVelocity = c.GetCurrentVelocity();
			if (currentVelocity.y < 0.0f)	// falling?
			{
				glm::vec3 hitPos, hitNormal;
				float hitDepth;
				if (physics->SweepCapsule(c.GetRadius(), c.GetHalfHeight(), worldSpacePos, t.GetOrientation(), { 0.0f,-1.0f,0.0f }, -currentVelocity.y * timeDelta, hitPos, hitNormal, hitDepth))
				{
					graphics->DebugRenderer().DrawLine(hitPos, hitPos + hitNormal * 50.0f, { 1.0f,0.0f,0.0f,1.0f });

					if (hitDepth < 0.0f)
					{
						// we were already overlapping something, try to push ourself up (but don't touch velocity!)
						newPosition.y += -hitDepth;
					}
					else
					{
						currentVelocity.y = 0.0f;
					}
				}
				else
				{
					newPosition = t.GetPosition() + currentVelocity * timeDelta;
				}
			}

			// gravity
			currentVelocity += gravity * timeDelta;
			c.SetCurrentVelocity(currentVelocity);

			// always update?
			t.SetPosition(newPosition);
		});

		return true;
	}

	void CharacterControllerSystem::Shutdown()
	{
		SDE_PROF_EVENT();
	}
}