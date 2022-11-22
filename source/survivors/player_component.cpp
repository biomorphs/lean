#include "player_component.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(PlayerComponent, 
	"GetCurrentHealth", &PlayerComponent::GetCurrentHealth,
	"SetCurrentHealth", &PlayerComponent::SetCurrentHealth,
	"GetMaximumHealth", &PlayerComponent::GetMaximumHealth,
	"SetMaximumHealth", &PlayerComponent::SetMaximumHealth,
	"GetCurrentXP", &PlayerComponent::GetCurrentXP,
	"SetCurrentXP", &PlayerComponent::SetCurrentXP,
	"GetThisLevelXP", &PlayerComponent::GetThisLevelXP,
	"SetThisLevelXP", &PlayerComponent::SetThisLevelXP,
	"GetNextLevelXP", &PlayerComponent::GetNextLevelXP,
	"SetNextLevelXP", &PlayerComponent::SetNextLevelXP,
	"GetAreaMultiplier", &PlayerComponent::GetAreaMultiplier,
	"SetAreaMultiplier", &PlayerComponent::SetAreaMultiplier,
	"GetDamageMultiplier", &PlayerComponent::GetDamageMultiplier,
	"SetDamageMultiplier", &PlayerComponent::SetDamageMultiplier,
	"GetCooldownMultiplier", &PlayerComponent::GetCooldownMultiplier,
	"SetCooldownMultiplier", &PlayerComponent::SetCooldownMultiplier
)
SERIALISE_BEGIN(PlayerComponent)
SERIALISE_PROPERTY("CurrentHP", m_currentHP)
SERIALISE_PROPERTY("MaxHP", m_maximumHP)
SERIALISE_PROPERTY("CurrentXP", m_currentXP)
SERIALISE_PROPERTY("ThisLevelXP", m_thisLevelXP)
SERIALISE_PROPERTY("NextLevelXP", m_nextLevelXP)
SERIALISE_PROPERTY("AreaMulti", m_multiArea)
SERIALISE_PROPERTY("DamageMulti", m_multiDamage)
SERIALISE_PROPERTY("CooldownMulti", m_multiCooldown)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(PlayerComponent)
{
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto fn = [gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& a = *static_cast<StorageType&>(cs).Find(e);
		i.Inspect("Max HP", a.GetMaximumHealth(), InspectFn(e, &PlayerComponent::SetMaximumHealth));
		i.Inspect("Current HP", a.GetCurrentHealth(), InspectFn(e, &PlayerComponent::SetCurrentHealth));
		i.Inspect("Current XP", a.GetCurrentXP(), InspectFn(e, &PlayerComponent::SetCurrentXP));
		i.Inspect("This Level XP", a.GetThisLevelXP(), InspectFn(e, &PlayerComponent::SetThisLevelXP));
		i.Inspect("Next Level XP", a.GetNextLevelXP(), InspectFn(e, &PlayerComponent::SetNextLevelXP));
		i.Inspect("Area Multi", a.GetAreaMultiplier(), InspectFn(e, &PlayerComponent::SetAreaMultiplier));
		i.Inspect("Damage Multi", a.GetDamageMultiplier(), InspectFn(e, &PlayerComponent::SetDamageMultiplier));
		i.Inspect("Cooldown Multi", a.GetCooldownMultiplier(), InspectFn(e, &PlayerComponent::SetCooldownMultiplier));
	};
	return fn;
}