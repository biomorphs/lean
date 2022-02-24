#include "component_character_controller.h"
#include "component_transform.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_render.h"
#include "entity/entity_handle.h"
#include "entity/world.h"
#include "engine/graphics_system.h"
#include "entity/world.h"

COMPONENT_SCRIPTS(CharacterController,
	"GetRadius", &CharacterController::GetRadius,
	"SetRadius", &CharacterController::SetRadius,
	"GetHalfHeight", &CharacterController::GetHalfHeight,
	"SetHalfHeight", &CharacterController::SetHalfHeight,
	"GetVerticalOffset", &CharacterController::GetVerticalOffset,
	"SetVerticalOffset", &CharacterController::SetVerticalOffset,
	"GetCurrentVelocity", &CharacterController::GetCurrentVelocity,
	"SetCurrentVelocity", &CharacterController::SetCurrentVelocity
)

SERIALISE_BEGIN(CharacterController)
SERIALISE_PROPERTY("CurrentVelocity", m_currentVelocity)
SERIALISE_PROPERTY("Radius", m_capsuleRadius)
SERIALISE_PROPERTY("HalfHeight", m_capsuleHalfHeight)
SERIALISE_PROPERTY("VerticalOffset", m_verticalOffset)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(CharacterController)
{
	auto fn = [](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& cc = *static_cast<CharacterController::StorageType&>(cs).Find(e);

		auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();

		gui->DragVector("Velocity", cc.GetCurrentVelocity());
		cc.SetRadius(gui->DragFloat("Capsule Radius", cc.GetRadius(), 0.1f, 0.1f));
		cc.SetHalfHeight(gui->DragFloat("Half Height", cc.GetHalfHeight(), 0.1f, 0.1f));
		cc.SetVerticalOffset(gui->DragFloat("Vertical Offset", cc.GetVerticalOffset(), 0.1f));
		
		glm::mat4 capsuleTransform = glm::identity<glm::mat4>();
		auto transform = world->GetComponent<Transform>(e);
		if (transform)
		{
			// ignore scale since we dont pass it to physx
			// also ignore any parent transform!
			capsuleTransform = glm::translate(glm::identity<glm::mat4>(), transform->GetPosition() + glm::vec3(0.0f, cc.GetVerticalOffset(),0.0f));
			capsuleTransform = capsuleTransform * glm::toMat4(transform->GetOrientation());
		}
		graphics->DebugRenderer().DrawCapsule(cc.GetRadius(), cc.GetHalfHeight(), { 0.0f,1.0f,1.0f,1.0f }, capsuleTransform);
	};
	return fn;
}