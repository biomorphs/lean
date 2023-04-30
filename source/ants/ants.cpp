#include "ants.h"
#include "core/file_io.h"
#include "core/random.h"
#include "entity/entity_system.h"
#include "entity/component.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_physics.h"
#include "engine/components/component_tags.h"
#include "behaviours/behaviour_tree_instance.h"
#include "behaviours/behaviour_tree_system.h"

class AntFoodComponent
{
public:
	COMPONENT(AntFoodComponent);
	COMPONENT_INSPECTOR();
	void SetFoodAmount(float a) { m_foodAmount = a; }
	float m_foodAmount = 8.0f;
};
COMPONENT_SCRIPTS(AntFoodComponent,
	"m_foodAmount", &AntFoodComponent::m_foodAmount
)
SERIALISE_BEGIN(AntFoodComponent)
SERIALISE_PROPERTY("FoodAmount", m_foodAmount)
SERIALISE_END()
COMPONENT_INSPECTOR_IMPL(AntFoodComponent)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		static EntityHandle setParentEntity;
		auto& t = *static_cast<AntFoodComponent::StorageType&>(cs).Find(e);
		i.Inspect("Food Amount", t.m_foodAmount, InspectFn(e, &AntFoodComponent::SetFoodAmount), 1.0f, 0.0f);
	};
	return fn;
}

class AntComponent
{
public:
	COMPONENT(AntComponent);
	COMPONENT_INSPECTOR();
	void SetBehaviourTree(std::string p) { m_behaviourTree = p; }
	void SetEnergy(float e) { m_energy = e; }
	void SetMinEnergyForHunger(float e) { m_minEnergyForHunger = e; }
	void SetIdleEnergyUsage(float f) { m_idleEnergyUsage = f; }
	void SetWalkingEnergyUsage(float f) { m_walkingEnergyUsage = f; }
	float GetHungerAmount() {
		if (!GetIsHungry())
		{
			return 0.0f;
		}
		else
		{
			return 1.0f - (m_energy / (m_minEnergyForHunger + 0.01f));	// 0.01f to avoid /0
		}
	}
	bool GetIsHungry() { return m_energy <= m_minEnergyForHunger; }

	std::string m_behaviourTree = "";
	Behaviours::BehaviourTreeInstance* m_treeInstance;
	float m_energy = 100.0f;
	float m_minEnergyForHunger = 50.0f;
	float m_walkingEnergyUsage = 0.03f;
	float m_idleEnergyUsage = 0.01f;
	int m_maxHeldItems = 4;
	std::vector<EntityHandle> m_carriedItems;	// ants can carry items
};
COMPONENT_SCRIPTS(AntComponent,
	"m_behaviourTree", &AntComponent::m_behaviourTree,
	"m_energy", &AntComponent::m_energy,
	"m_minEnergyForHunger", &AntComponent::m_minEnergyForHunger
)
SERIALISE_BEGIN(AntComponent)
SERIALISE_PROPERTY("BehaviourTree", m_behaviourTree)
SERIALISE_PROPERTY("Energy", m_energy)
SERIALISE_PROPERTY("MinEnergyForHunger", m_minEnergyForHunger)
SERIALISE_PROPERTY("WalkEnergyUsage", m_walkingEnergyUsage)
SERIALISE_PROPERTY("IdleEnergyUsage", m_idleEnergyUsage)
SERIALISE_PROPERTY("MaxHeldItems", m_maxHeldItems)
SERIALISE_PROPERTY("HeldItems", m_carriedItems)
SERIALISE_END()
COMPONENT_INSPECTOR_IMPL(AntComponent)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		static EntityHandle setParentEntity;
		auto& t = *static_cast<AntComponent::StorageType&>(cs).Find(e);
		i.InspectFilePath("Behavior Tree", "beht", t.m_behaviourTree, InspectFn(e, &AntComponent::SetBehaviourTree));
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		if (dbgGui->Button("Debug"))
		{
			auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
			behSys->DebugInstance(t.m_treeInstance);
		}
		i.Inspect("Energy", t.m_energy, InspectFn(e, &AntComponent::SetEnergy));
		i.Inspect("Hunger energy threshold", t.m_minEnergyForHunger, InspectFn(e, &AntComponent::SetMinEnergyForHunger));
	};
	return fn;
}

class AntPickupComponent
{
public:
	COMPONENT(AntPickupComponent);
	COMPONENT_INSPECTOR();
	void SetHeldBy(EntityHandle a) { m_heldBy = a; }
	EntityHandle m_heldBy;	// what is holding thisitem
};
COMPONENT_SCRIPTS(AntPickupComponent,
	"m_heldBy", &AntPickupComponent::m_heldBy
)
SERIALISE_BEGIN(AntPickupComponent)
SERIALISE_PROPERTY("HeldBy", m_heldBy)
SERIALISE_END()
COMPONENT_INSPECTOR_IMPL(AntPickupComponent)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		static EntityHandle setParentEntity;
		auto& t = *static_cast<AntPickupComponent::StorageType&>(cs).Find(e);
		static auto entities = Engine::GetSystem<EntitySystem>("Entities");
		std::string heldByName = entities->GetEntityNameWithTags(t.m_heldBy);
		i.Inspect("Held By", t.m_heldBy, InspectFn(e, &AntPickupComponent::SetHeldBy), [&](const EntityHandle& ent) {
			return entities->GetWorld()->GetComponent<AntComponent>(ent) != nullptr;
		});
	};
	return fn;
}


namespace Behaviours
{
	class AntIdleNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		AntIdleNode()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Blackboard entity", m_targetEntityInBB);
		}
		virtual std::string_view GetTypeName() const { return "Ants.AntIdle"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetEntityInBB.c_str());
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
			Transform* targetAntTransform = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			if (targetAntCmp)
			{
				targetAntCmp->m_energy = glm::max(0.0f,targetAntCmp->m_energy - targetAntCmp->m_idleEnergyUsage);
				glm::quat rotation(glm::vec3(0.0f, 0.01f, 0.0f));
				targetAntTransform->SetOrientation(targetAntTransform->GetOrientation() * rotation);
			}
			return RunningState::Success;
		}

		std::string m_targetEntityInBB;
	};
	SERIALISE_BEGIN_WITH_PARENT(AntIdleNode, Node)
	SERIALISE_PROPERTY("TargetEntityInBB", m_targetEntityInBB)
	SERIALISE_END();

	class GetAntHungerNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		GetAntHungerNode()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target entity", m_targetEntityInBB);
			dbg.TextInput("Target blackboard value", m_targetHungerInBB);
		}
		virtual std::string_view GetTypeName() const { return "Ants.GetHunger"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetEntityInBB == "" || m_targetHungerInBB == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetEntityInBB.c_str());
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
			if (targetAntCmp && bti.m_bb.IsKey(m_targetHungerInBB))
			{
				bti.m_bb.SetFloat(m_targetHungerInBB.c_str(), targetAntCmp->GetHungerAmount());
			}
			return RunningState::Success;
		}

		std::string m_targetEntityInBB;
		std::string m_targetHungerInBB;
	};
	SERIALISE_BEGIN_WITH_PARENT(GetAntHungerNode, Node)
		SERIALISE_PROPERTY("TargetEntityInBB", m_targetEntityInBB)
		SERIALISE_PROPERTY("TargetHungerInBB", m_targetHungerInBB)
	SERIALISE_END();

	// returns failed if no food found
	class FindClosestFood : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		FindClosestFood()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
			dbg.TextInput("Food entity", m_foundEntity);
		}
		virtual std::string_view GetTypeName() const { return "Ants.FindClosestFood"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "" || m_foundEntity == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
			Transform* targetAntCmp = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			if (targetAntCmp && bti.m_bb.IsKey(m_foundEntity))
			{
				static auto foodIt = entities->GetWorld()->MakeIterator<AntFoodComponent, Transform>();
				glm::vec3 antPos = targetAntCmp->GetPosition();
				EntityHandle closestFound;
				float closestDistance = FLT_MAX;
				foodIt.ForEach([&](AntFoodComponent& afc, Transform& t, EntityHandle owner) {
					float distance = glm::distance(t.GetPosition(), antPos);
					if (distance < closestDistance)
					{
						auto pickupCmp = entities->GetWorld()->GetComponent<AntPickupComponent>(owner);
						if (pickupCmp && !pickupCmp->m_heldBy.IsValid())
						{
							closestFound = owner;
							closestDistance = distance;
						}
					}
				});
				bti.m_bb.SetEntity(m_foundEntity.c_str(), closestFound);
				return closestFound.GetID() != -1 ? RunningState::Success : RunningState::Failed;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_foundEntity;
	};
	SERIALISE_BEGIN_WITH_PARENT(FindClosestFood, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("FoundEntity", m_foundEntity)
	SERIALISE_END();

	class CanCarryMore : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		CanCarryMore()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
		}
		virtual std::string_view GetTypeName() const { return "Ants.CanCarryMore"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
			if (targetAntCmp)
			{
				return targetAntCmp->m_carriedItems.size() < targetAntCmp->m_maxHeldItems ? RunningState::Success : RunningState::Failed;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
	};
	SERIALISE_BEGIN_WITH_PARENT(CanCarryMore, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
	SERIALISE_END();

	class FindClosestTaggedEntity : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		FindClosestTaggedEntity()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Tag to match", m_searchTag);
			dbg.TextInput("Target Ant", m_targetAnt);
			dbg.TextInput("Found entity", m_foundEntity);
			m_allowPickedUp = dbg.Checkbox("Allow picked up items", m_allowPickedUp);
		}
		virtual std::string_view GetTypeName() const { return "Ants.FindClosestTaggedEntity"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "" || m_foundEntity == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
			Transform* targetAntCmp = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			if (targetAntCmp && bti.m_bb.IsKey(m_foundEntity))
			{
				glm::vec3 antPos = targetAntCmp->GetPosition();
				static auto tagIt = entities->GetWorld()->MakeIterator<Tags, Transform>();
				EntityHandle closestFound;
				float closestDistance = FLT_MAX;
				tagIt.ForEach([&](Tags& tags, Transform& t, EntityHandle owner) {
					float distance = glm::distance(t.GetPosition(), antPos);
					if (distance < closestDistance)
					{
						bool filteredByTag = !(m_searchTag.size() > 0 && tags.ContainsTag(m_searchTag.c_str()));
						bool filteredByPickup = false;
						if (!m_allowPickedUp)
						{
							auto pickupCmp = entities->GetWorld()->GetComponent<AntPickupComponent>(owner);
							if (pickupCmp && pickupCmp->m_heldBy.IsValid())		// already held
							{
								filteredByPickup = true;
							}
						}
						if (!filteredByTag && !filteredByPickup)
						{
							closestFound = owner;
							closestDistance = distance;
						}
					}
				});
				bti.m_bb.SetEntity(m_foundEntity.c_str(), closestFound);
				return closestFound.GetID() != -1 ? RunningState::Success : RunningState::Failed;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_foundEntity;
		std::string m_searchTag;
		bool m_allowPickedUp = true;
	};
	SERIALISE_BEGIN_WITH_PARENT(FindClosestTaggedEntity, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("FoundEntity", m_foundEntity)
		SERIALISE_PROPERTY("SearchTag", m_searchTag)
		SERIALISE_PROPERTY("AllowPickedUp", m_allowPickedUp)
	SERIALISE_END();

	// returns failed if the entity is not found, success when within the radius
	class WalkToEntity : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		WalkToEntity()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Walking entity", m_walker);
			dbg.TextInput("Target entity", m_target);
			dbg.TextInput("Walker Radius", m_walkerRadius);
			dbg.TextInput("Target Radius", m_targetRadius);
		}
		virtual std::string_view GetTypeName() const { return "Ants.WalkToEntity"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_walker == "" || m_target == "" || m_walkerRadius == "" || m_targetRadius == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle walker = bti.m_bb.TryGetEntity(m_walker.c_str());
			EntityHandle target = bti.m_bb.TryGetEntity(m_target.c_str());
			Transform* walkerCmp = entities->GetWorld()->GetComponent<Transform>(walker);
			Transform* targetCmp = entities->GetWorld()->GetComponent<Transform>(target);
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(walker);
			if (walkerCmp && targetCmp && targetAntCmp)
			{
				float distance = glm::distance(walkerCmp->GetPosition() * glm::vec3(1, 0, 1), targetCmp->GetPosition() * glm::vec3(1, 0, 1));
				float walkerRadius = bti.m_bb.GetOrParseFloat(m_walkerRadius.c_str());
				float targetRadius = bti.m_bb.GetOrParseFloat(m_targetRadius.c_str());
				if (distance > (walkerRadius + targetRadius))
				{
					glm::vec3 dir = glm::normalize((targetCmp->GetPosition() - walkerCmp->GetPosition()) * glm::vec3(1, 0, 1));
					walkerCmp->SetPosition(walkerCmp->GetPosition() + dir * 8.0f * 0.033f);		// todo walk speed and delta
					targetAntCmp->m_energy = glm::max(0.0f, targetAntCmp->m_energy - targetAntCmp->m_walkingEnergyUsage);
					return RunningState::Running;
				}
				else
				{
					return RunningState::Success;
				}
			}
			return RunningState::Failed;
		}

		std::string m_walker;
		std::string m_target;
		std::string m_walkerRadius = "1.0f";
		std::string m_targetRadius = "1.0f";
	};
	SERIALISE_BEGIN_WITH_PARENT(WalkToEntity, Node)
		SERIALISE_PROPERTY("WalkerEntity", m_walker)
		SERIALISE_PROPERTY("TargetEntity", m_target)
		SERIALISE_PROPERTY("WalkerRadius", m_walkerRadius)
		SERIALISE_PROPERTY("TargetRadius", m_targetRadius)
	SERIALISE_END();

	class EatUntilFullNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		EatUntilFullNode()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
			dbg.TextInput("Food entity", m_foodEntity);
		}
		virtual std::string_view GetTypeName() const { return "Ants.EatUntilFull"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "" || m_foodEntity == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
			Transform* targetAntTransform = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
			EntityHandle foodEntity = bti.m_bb.TryGetEntity(m_foodEntity.c_str());
			Transform* foodTransform = entities->GetWorld()->GetComponent<Transform>(foodEntity);
			AntFoodComponent* foodCmp = entities->GetWorld()->GetComponent<AntFoodComponent>(foodEntity);
			if (targetAntTransform && targetAntCmp && foodTransform && foodCmp)
			{
				if (targetAntCmp->m_energy >= 100.0f)
				{
					return RunningState::Success;
				}
				if (glm::distance(targetAntTransform->GetPosition(), foodTransform->GetPosition()) > 10.0f)
				{
					return RunningState::Failed;
				}

				float eatAmount = glm::min(1.0f * 0.033f, foodCmp->m_foodAmount);	// todo delta/eat speed
				foodCmp->m_foodAmount = glm::max(0.0f, foodCmp->m_foodAmount - eatAmount);
				targetAntCmp->m_energy = targetAntCmp->m_energy + eatAmount;

				if (foodCmp->m_foodAmount <= 0.0f)
				{
					entities->GetWorld()->RemoveEntity(foodEntity);
				}

				return RunningState::Running;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_foodEntity;
	};
	SERIALISE_BEGIN_WITH_PARENT(EatUntilFullNode, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("FoodEntity", m_foodEntity)
	SERIALISE_END();

	class PickUpItemNode : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		PickUpItemNode()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_ant);
			dbg.TextInput("Object to pick up", m_pickup);
		}
		virtual std::string_view GetTypeName() const { return "Ants.PickUpItem"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_ant == "" || m_pickup == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle ant = bti.m_bb.TryGetEntity(m_ant.c_str());
			EntityHandle item = bti.m_bb.TryGetEntity(m_pickup.c_str());
			auto pickupCmp = entities->GetWorld()->GetComponent<AntPickupComponent>(item);
			auto antCmp = entities->GetWorld()->GetComponent<AntComponent>(ant);
			auto antTransform = entities->GetWorld()->GetComponent<Transform>(ant);
			auto pickupTransform = entities->GetWorld()->GetComponent<Transform>(item);
			if (pickupCmp && antCmp && antTransform && pickupTransform && !pickupCmp->m_heldBy.IsValid() && antCmp->m_carriedItems.size() < antCmp->m_maxHeldItems)
			{
				glm::vec3 invScale= 1.0f / (antTransform->GetScale());
				pickupTransform->SetScale(pickupTransform->GetScale() * invScale);
				glm::vec3 newPos = { Core::Random::GetFloat(-5.0f,5.0f), 5.0f + antCmp->m_carriedItems.size() * 4.0f, Core::Random::GetFloat(-10.0f,10.0f) };
				newPos *= invScale;
				pickupTransform->SetPosition(newPos);
				pickupTransform->SetParent(ant);
				pickupCmp->m_heldBy = ant;
				antCmp->m_carriedItems.push_back(item);

				auto pickupPhysCmp = entities->GetWorld()->GetComponent<Physics>(item);
				if (pickupPhysCmp)
				{
					pickupPhysCmp->SetStatic(false);
					pickupPhysCmp->SetKinematic(true);
					pickupPhysCmp->Rebuild();
				}

				return RunningState::Success;
			}
			return RunningState::Failed;
		}
		std::string m_ant;
		std::string m_pickup;
	};
	SERIALISE_BEGIN_WITH_PARENT(PickUpItemNode, Node)
		SERIALISE_PROPERTY("Ant", m_ant)
		SERIALISE_PROPERTY("PickupItem", m_pickup)
	SERIALISE_END();
}

bool AntsSystem::PostInit()
{
	SDE_PROF_EVENT();
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	entities->RegisterComponentType<AntFoodComponent>();
	entities->RegisterInspector<AntFoodComponent>(AntFoodComponent::MakeInspector());
	entities->RegisterComponentType<AntComponent>();
	entities->RegisterInspector<AntComponent>(AntComponent::MakeInspector());
	entities->RegisterComponentType<AntPickupComponent>();
	entities->RegisterInspector<AntPickupComponent>(AntPickupComponent::MakeInspector());

	auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
	behSys->RegisterNodeType("Ants.AntIdle", []() {
		return std::make_unique<Behaviours::AntIdleNode>();
	});
	behSys->RegisterNodeType("Ants.GetAntHunger", []() {
		return std::make_unique<Behaviours::GetAntHungerNode>();
	});
	behSys->RegisterNodeType("Ants.FindClosestFood", []() {
		return std::make_unique<Behaviours::FindClosestFood>();
	});
	behSys->RegisterNodeType("Ants.WalkToEntity", []() {
		return std::make_unique<Behaviours::WalkToEntity>();
	});
	behSys->RegisterNodeType("Ants.EatUntilFullNode", []() {
		return std::make_unique<Behaviours::EatUntilFullNode>();
	});
	behSys->RegisterNodeType("Ants.PickUpItemNode", []() {
		return std::make_unique<Behaviours::PickUpItemNode>();
	});
	behSys->RegisterNodeType("Ants.FindClosestTaggedEntity", []() {
		return std::make_unique<Behaviours::FindClosestTaggedEntity>();
	});
	behSys->RegisterNodeType("Ants.CanCarryMore", []() {
		return std::make_unique<Behaviours::CanCarryMore>();
	});
	return true;
}

void AntsSystem::KillAnt(const EntityHandle& e)
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
	auto ac = entities->GetWorld()->GetComponent<AntComponent>(e);
	auto antTrans = entities->GetWorld()->GetComponent<Transform>(e);
	if (ac && antTrans)
	{
		// drop all held items
		for (int he = 0; he < ac->m_carriedItems.size(); he++)
		{
			EntityHandle heldEntity = ac->m_carriedItems[he];
			auto heldTrns = entities->GetWorld()->GetComponent<Transform>(heldEntity);
			auto pickupCmp = entities->GetWorld()->GetComponent<AntPickupComponent>(heldEntity);
			auto pickupPhysCmp = entities->GetWorld()->GetComponent<Physics>(heldEntity);
			if (pickupPhysCmp)
			{
				pickupPhysCmp->SetKinematic(false);
				pickupPhysCmp->Rebuild();
			}
			if (pickupCmp)
			{
				pickupCmp->m_heldBy = EntityHandle();
			}
			glm::vec3 wsPos, wsScale;
			glm::quat wsOrientation;
			heldTrns->GetWorldSpaceTransform(wsPos, wsOrientation, wsScale);
			heldTrns->SetParent(EntityHandle());
			heldTrns->SetScale(wsScale);
			heldTrns->SetPosition(wsPos);
			heldTrns->SetOrientation(wsOrientation);
		}

		behSys->DestroyInstance(ac->m_treeInstance);
		ac->m_treeInstance = nullptr;
	}

	entities->GetWorld()->RemoveEntity(e);
}

bool AntsSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
	entities->GetWorld()->ForEachComponent<AntComponent>([&](AntComponent& ac, EntityHandle e) {
		if (ac.m_treeInstance == nullptr && ac.m_behaviourTree.size() > 0)
		{
			ac.m_treeInstance = behSys->CreateTreeInstance(ac.m_behaviourTree);
			if (ac.m_treeInstance)
			{
				ac.m_treeInstance->m_bb.SetEntity("@OwnerEntity", e);
			}
		}
		if (ac.m_treeInstance)
		{
			ac.m_treeInstance->Tick();
		}
		if (ac.m_energy == 0.0f)
		{
			KillAnt(e);
		}
	});

	return true;
}