#include "attract_to_entity_component.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(AttractToEntityComponent,
	"SetTarget", &AttractToEntityComponent::SetTarget,
	"SetTargetOffset", &AttractToEntityComponent::SetTargetOffset,
	"SetAcceleration", &AttractToEntityComponent::SetAcceleration,
	"SetMaxSpeed", &AttractToEntityComponent::SetMaxSpeed,
	"SetTriggerRange", &AttractToEntityComponent::SetTriggerRange,
	"SetActivationRange", &AttractToEntityComponent::SetActivationRange,
	"SetTriggerCallback", &AttractToEntityComponent::SetTriggerCallback
)

// trigger/cb can't be serialised. sadface
SERIALISE_BEGIN(AttractToEntityComponent)
SERIALISE_PROPERTY("Target", m_target)
SERIALISE_PROPERTY("TargetOffset", m_targetOffset)
SERIALISE_PROPERTY("Acceleration", m_acceleration)
SERIALISE_PROPERTY("MaxSpeed", m_maxSpeed)
SERIALISE_PROPERTY("TriggerRange", m_triggerRange)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(AttractToEntityComponent)
{
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto fn = [gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& a = *static_cast<StorageType&>(cs).Find(e);
		auto entities = Engine::GetSystem<EntitySystem>("Entities");

		i.Inspect("Target", a.GetTargetEntity(), InspectFn(e, &AttractToEntityComponent::SetTarget), [entities](const EntityHandle& p) {
			return entities->GetWorld()->GetComponent<Transform>(p) != nullptr;
		});
		i.Inspect("Acceleration", a.GetAcceleration(), InspectFn(e, &AttractToEntityComponent::SetAcceleration));
		i.Inspect("Max Speed", a.GetMaxSpeed(), InspectFn(e, &AttractToEntityComponent::SetMaxSpeed));
		i.Inspect("Trigger Range", a.GetTriggerRange(), InspectFn(e, &AttractToEntityComponent::SetTriggerRange));
	};
	return fn;
}