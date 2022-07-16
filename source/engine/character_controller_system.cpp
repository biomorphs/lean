#include "character_controller_system.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "debug_gui_menubar.h"
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
		auto dbgGui = Engine::GetSystem<DebugGuiSystem>("DebugGui");
		auto world = entities->GetWorld();

		Engine::MenuBar mainMenu;
		auto& physicsMenu = mainMenu.AddSubmenu(ICON_FK_CUBE " Physics");
		auto& cctMenu = physicsMenu.AddSubmenu("Character Controllers");
		cctMenu.AddItem(m_enableDebug ? "Disable Debug" : "Enable Debug", [this]() {
			m_enableDebug = !m_enableDebug;
		});
		dbgGui->MainMenuBar(mainMenu);
		
		// get global gravity value from world
		glm::vec3 gravity(0.0f, -9.8f, 0.0f);
		world->ForEachComponent<EnvironmentSettings>([&gravity](EnvironmentSettings& s, EntityHandle owner) {
			gravity = s.GetGravity();
		});
		
		static auto s_it = world->MakeIterator<CharacterController, Transform>();
		s_it.ForEach([this, graphics, physics, timeDelta, gravity](CharacterController& c, Transform& t, EntityHandle e) {
			if (!c.GetEnabled())
			{
				return;
			}
			glm::mat4 matrix = glm::translate(glm::identity<glm::mat4>(), t.GetPosition() + glm::vec3(0.0f,c.GetVerticalOffset(),0.0f));
			matrix = matrix * glm::toMat4(t.GetOrientation());
			glm::vec3 worldSpacePos = glm::vec3(matrix[3]);
			glm::vec4 capsuleColour(0.0f, 0.2f, 0.2f, 1.0f);
			glm::vec3 newPosition = t.GetPosition();
			glm::vec3 currentVelocity = c.GetCurrentVelocity();
			glm::vec3 up(0.0f, 1.0f, 0.0f);
			if (currentVelocity.y < 0.0f)	// falling?
			{
				glm::vec3 hitPos, hitNormal;
				float hitDepth;
				EntityHandle hitEntity;
				bool hit = physics->SweepCapsule(c.GetRadius(), c.GetHalfHeight(), worldSpacePos, t.GetOrientation(), { 0.0f,-1.0f,0.0f }, -currentVelocity.y * timeDelta, hitPos, hitNormal, hitDepth, hitEntity);
				if (hit && hitEntity.GetID() != e.GetID())
				{
					capsuleColour = { 0.0f,0.5f,0.5f,1.0f };
					if (m_enableDebug)
					{
						graphics->DebugRenderer().DrawLine(hitPos, hitPos + hitNormal * 50.0f, { 1.0f,1.0f,1.0f,1.0f });
					}

					if (hitDepth < 0.0f)
					{
						// we were already overlapping something, try to push ourself up (but don't touch velocity!)
						//newPosition.y += -hitDepth;
						newPosition = newPosition  + -hitDepth * hitNormal;
						capsuleColour = { 1.0f,0.0f,0.0f,1.0f };
					}
					else
					{
						const float epsilon = 0.001f;
						if (hitDepth < epsilon)
						{
							// almost, or actually touching something
							float slopeAngle = glm::dot(hitNormal, up);
							//SDE_LOG("touching something, slope=%f", slopeAngle);
						}
						// we are about to hit something
						currentVelocity.y = 0.0f;
					}
				}
				else
				{
					newPosition = t.GetPosition() + currentVelocity * timeDelta;
				}
			}

			if (m_enableDebug)
			{
				graphics->DebugRenderer().DrawCapsule(c.GetRadius(), c.GetHalfHeight(), capsuleColour, matrix);
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