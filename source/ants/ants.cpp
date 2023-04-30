#include "ants.h"
#include "core/file_io.h"
#include "core/random.h"
#include "entity/entity_system.h"
#include "entity/component.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/system_manager.h"
#include "engine/raycast_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_physics.h"
#include "engine/components/component_tags.h"
#include "behaviours/behaviour_tree_instance.h"
#include "behaviours/behaviour_tree_system.h"

const float c_nestRadius = 64.0f;
const float c_queenTowerRadius = 48.0f;

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
	virtual ~AntComponent()
	{
		if (m_treeInstance != nullptr)
		{
			auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
			behSys->DestroyInstance(m_treeInstance);
		}
	}
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
	Behaviours::BehaviourTreeInstance* m_treeInstance = nullptr;
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
			m_allowCloseToNest = dbg.Checkbox("Allow items close to nest", m_allowCloseToNest);
			if (!m_allowCloseToNest)
			{
				dbg.TextInput("Nest entity", m_antNestEntity);
			}
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
			EntityHandle nestEntity = bti.m_bb.TryGetEntity(m_antNestEntity.c_str());
			Transform* targetAntCmp = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			Transform* nestTransform = entities->GetWorld()->GetComponent<Transform>(nestEntity);
			if (targetAntCmp && bti.m_bb.IsKey(m_foundEntity))
			{
				glm::vec3 antPos = targetAntCmp->GetPosition();
				static auto tagIt = entities->GetWorld()->MakeIterator<Tags, Transform>();
				EntityHandle closestFound;
				float closestDistance = FLT_MAX;
				const Engine::Tag searchTag(m_searchTag.c_str());
				tagIt.ForEach([&](Tags& tags, Transform& t, EntityHandle owner) {
					bool filteredByTag = (m_searchTag.size() > 0 && !tags.ContainsTag(searchTag));
					if (filteredByTag)
					{
						return;
					}
					float distance = glm::distance(t.GetPosition(), antPos);
					if (distance >= closestDistance)
					{
						return;
					}
					if (!m_allowCloseToNest && nestTransform)
					{
						if (glm::distance(nestTransform->GetPosition(), t.GetPosition()) < (c_nestRadius * 1.8f))
						{
							return;
						}
					}
					if (!m_allowPickedUp)
					{
						auto pickupCmp = entities->GetWorld()->GetComponent<AntPickupComponent>(owner);
						if (pickupCmp && pickupCmp->m_heldBy.IsValid())		// already held
						{
							return;
						}
					}
					closestFound = owner;
					closestDistance = distance;
				});
				bti.m_bb.SetEntity(m_foundEntity.c_str(), closestFound);
				return closestFound.GetID() != -1 ? RunningState::Success : RunningState::Failed;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_foundEntity;
		std::string m_searchTag;
		std::string m_antNestEntity;
		bool m_allowPickedUp = true;
		bool m_allowCloseToNest = false;
	};
	SERIALISE_BEGIN_WITH_PARENT(FindClosestTaggedEntity, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("FoundEntity", m_foundEntity)
		SERIALISE_PROPERTY("SearchTag", m_searchTag)
		SERIALISE_PROPERTY("AntNestEntity", m_antNestEntity)
		SERIALISE_PROPERTY("AllowPickedUp", m_allowPickedUp)
		SERIALISE_PROPERTY("AllowCloseToNest", m_allowCloseToNest)
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
			dbg.TextInput("Target entity/vector", m_target);
			dbg.TextInput("Walker Radius", m_walkerRadius);
			dbg.TextInput("Target Radius", m_targetRadius);
			m_cancelIfHungry = dbg.Checkbox("Cancel if hungry?", m_cancelIfHungry);
			m_cancelIfTargetPickedUp = dbg.Checkbox("Cancel if target picked up?", m_cancelIfTargetPickedUp);
		}
		virtual std::string_view GetTypeName() const { return "Ants.WalkTo"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_walker == "" || m_target == "" || m_walkerRadius == "" || m_targetRadius == "")
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
			EntityHandle walker = bti.m_bb.TryGetEntity(m_walker.c_str());
			EntityHandle target = bti.m_bb.TryGetEntity(m_target.c_str());
			Transform* walkerCmp = entities->GetWorld()->GetComponent<Transform>(walker);
			Transform* targetCmp = entities->GetWorld()->GetComponent<Transform>(target);
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(walker);
			glm::vec3 targetPosition = targetCmp ? glm::vec3(targetCmp->GetWorldspaceMatrix()[3]) : bti.m_bb.TryGetVector(m_target.c_str());
			if (walkerCmp && targetAntCmp)
			{
				if (m_cancelIfHungry && targetAntCmp->GetIsHungry())
				{
					return RunningState::Failed;
				}
				if (m_cancelIfTargetPickedUp)
				{
					auto targetPickup = entities->GetWorld()->GetComponent<AntPickupComponent>(target);
					if (targetPickup && targetPickup->m_heldBy.IsValid())
					{
						return RunningState::Failed;
					}
				}
				float distance = glm::distance(walkerCmp->GetPosition() * glm::vec3(1, 0, 1), targetPosition * glm::vec3(1, 0, 1));
				float walkerRadius = bti.m_bb.GetOrParseFloat(m_walkerRadius.c_str());
				float targetRadius = bti.m_bb.GetOrParseFloat(m_targetRadius.c_str());
				if (distance > (walkerRadius + targetRadius))
				{
					// glm::vec4 lineColour = targetCmp != nullptr ? glm::vec4{ 1, 1, 0, 1 } : glm::vec4{ 0, 1,1,1};
					// graphics->DebugRenderer().DrawLine(walkerCmp->GetPosition() + glm::vec3{0.0, 10.0f, 0.0}, targetPosition + glm::vec3{0.0, 10.0f, 0.0}, lineColour);
					glm::vec3 dir = glm::normalize((targetPosition - walkerCmp->GetPosition()) * glm::vec3(1, 0, 1));
					float weightSpeedMul = 1.0f - ((float)targetAntCmp->m_carriedItems.size() / (float)targetAntCmp->m_maxHeldItems);
					walkerCmp->SetPosition(walkerCmp->GetPosition() + dir * (24.0f + weightSpeedMul * 32.0f) * 0.016f);		// todo walk speed and delta
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
		bool m_cancelIfHungry = false;
		bool m_cancelIfTargetPickedUp = false;
	};
	SERIALISE_BEGIN_WITH_PARENT(WalkToEntity, Node)
		SERIALISE_PROPERTY("WalkerEntity", m_walker)
		SERIALISE_PROPERTY("TargetEntity", m_target)
		SERIALISE_PROPERTY("WalkerRadius", m_walkerRadius)
		SERIALISE_PROPERTY("TargetRadius", m_targetRadius)
		SERIALISE_PROPERTY("CancelIfHungry", m_cancelIfHungry)
		SERIALISE_PROPERTY("CancelIfTargetPickedUp", m_cancelIfTargetPickedUp)
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

				float eatAmount = glm::min(8.0f * 0.016f, foodCmp->m_foodAmount);	// todo delta/eat speed
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

	class FindDropoffPosition : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		FindDropoffPosition()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
			dbg.TextInput("Ant Nest Entity", m_antNestEntity);
			dbg.TextInput("Dropoff Target Vector", m_dropoffTarget);
		}
		virtual std::string_view GetTypeName() const { return "Ants.FindDropoffPosition"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "" || m_dropoffTarget == "" || !bti.m_bb.IsKey(m_dropoffTarget))
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
			EntityHandle nest = bti.m_bb.TryGetEntity(m_antNestEntity.c_str());
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
			Transform* nestTransform = entities->GetWorld()->GetComponent<Transform>(nest);
			Transform* antTransform = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			if (targetAntCmp)
			{
				if (nestTransform)
				{
					glm::vec3 nestToAnt = glm::normalize(antTransform->GetPosition() - nestTransform->GetPosition());
					bti.m_bb.SetVector(m_dropoffTarget.c_str(), nestTransform->GetPosition() + nestToAnt * Core::Random::GetFloat(c_nestRadius * 1.0f, c_nestRadius* 1.5f));
				}
				else
				{
					float t = Core::Random::GetFloat(0.0f, glm::pi<float>() * 2.0f);
					glm::vec3 randPosOnCircle = { sin(t), 0.0f, cos(t) };
					randPosOnCircle *= c_nestRadius * 1.5f;
					bti.m_bb.SetVector(m_dropoffTarget.c_str(), randPosOnCircle);
				}
				return RunningState::Success;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_antNestEntity;
		std::string m_dropoffTarget;
	};
	SERIALISE_BEGIN_WITH_PARENT(FindDropoffPosition, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("AntNest", m_antNestEntity)
		SERIALISE_PROPERTY("DropoffTarget", m_dropoffTarget)
	SERIALISE_END();

	class FindBuildPosition : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		FindBuildPosition()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
			dbg.TextInput("Queen Entity", m_queenEntity);
			dbg.TextInput("Build Target Vector", m_buildTarget);
		}
		virtual std::string_view GetTypeName() const { return "Ants.FindBuildPosition"; }
		virtual RunningState Tick(RunningNodeContext*, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "" || m_buildTarget == "" || !bti.m_bb.IsKey(m_buildTarget))
			{
				return RunningState::Failed;
			}
			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
			EntityHandle queen = bti.m_bb.TryGetEntity(m_queenEntity.c_str());
			AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
			Transform* queenTransform = entities->GetWorld()->GetComponent<Transform>(queen);
			Transform* antTransform = entities->GetWorld()->GetComponent<Transform>(targetAnt);
			if (targetAntCmp && queenTransform)
			{
				float t = Core::Random::GetFloat(0.0f, glm::pi<float>() * 2.0f);
				glm::vec3 randPosOnCircle = { sin(t), 0.0f, cos(t) };
				randPosOnCircle *= c_queenTowerRadius * 1.5f;
				bti.m_bb.SetVector(m_buildTarget.c_str(), glm::vec3(queenTransform->GetWorldspaceMatrix()[3]) + randPosOnCircle);
				return RunningState::Success;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_queenEntity;
		std::string m_buildTarget;
	};
	SERIALISE_BEGIN_WITH_PARENT(FindBuildPosition, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("QueenEntity", m_queenEntity)
		SERIALISE_PROPERTY("BuildTarget", m_buildTarget)
	SERIALISE_END();

	class DropItems : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		DropItems()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
		}
		virtual std::string_view GetTypeName() const { return "Ants.DropItems"; }
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
				auto ants = Engine::GetSystem< AntsSystem>("Ants");
				ants->DropAllItems(targetAnt);
				return RunningState::Success;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
	};
	SERIALISE_BEGIN_WITH_PARENT(DropItems, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
	SERIALISE_END();

	// fires a raycast from the sky to determine build position for the held items
	class BuildWithItems : public Node
	{
	public:
		virtual SERIALISED_CLASS();
		BuildWithItems()
		{
			m_bgColour = { 0.8f,0.8f,0.0f };
			m_textColour = { 0,0,1 };
			m_editorDimensions = { 60, 32 };
		}
		virtual void ShowEditorGui(Engine::DebugGuiSystem& dbg)
		{
			dbg.TextInput("Target Ant", m_targetAnt);
			dbg.TextInput("Target Build Position", m_targetPosition);
		}
		struct BuildWithItemsContext : public RunningNodeContext {
			Engine::RaycastSystem::RayResultsFn m_rayResultFn;
			bool m_rayFired = false;
			bool m_resultReady = false;
			glm::vec3 m_rayStartPosition = glm::vec3(0, 0, 0);
			glm::vec3 m_rayHitPosition = glm::vec3(0, 0, 0);
		};
		virtual std::unique_ptr<RunningNodeContext> PrepareContext() const
		{
			return std::make_unique<BuildWithItemsContext>();
		}
		void Init(RunningNodeContext* ctx, BehaviourTreeInstance& bti) const
		{
			BuildWithItemsContext* buildCtx = static_cast<BuildWithItemsContext*>(ctx);
			buildCtx->m_rayFired = true;
			buildCtx->m_resultReady = false;
			buildCtx->m_rayHitPosition = { 0,0,0 };
			buildCtx->m_rayResultFn = [buildCtx](const std::vector<Engine::RaycastSystem::RayHitResult>& hits, const std::vector<Engine::RaycastSystem::RayMissResult>& misses)
			{
				if (hits.size() == 0)
				{
					buildCtx->m_rayHitPosition = buildCtx->m_rayStartPosition * glm::vec3{ 1,0,1 };
					buildCtx->m_rayHitPosition.y = Core::Random::GetFloat(4.0f, 32.0f);
				}
				else
				{
					buildCtx->m_rayHitPosition = hits[0].m_hitPos;
				}
				buildCtx->m_resultReady = true;
			};

			static auto entities = Engine::GetSystem<EntitySystem>("Entities");
			glm::vec3 buildPos = bti.m_bb.TryGetVector(m_targetPosition.c_str());

			static auto raycasts = Engine::GetSystem<Engine::RaycastSystem>("Raycasts");
			std::vector<Engine::RaycastSystem::RayInput> rayToCast(1);
			rayToCast[0].m_start = glm::vec3(buildPos) + glm::vec3(0.0f, 2000.0f, 0.0f);
			rayToCast[0].m_end = glm::vec3(0.0f,-16.0f,0.0f) + (buildPos * glm::vec3(1, 0, 1));
			buildCtx->m_rayStartPosition = rayToCast[0].m_start;
			raycasts->RaycastAsyncMulti(rayToCast, buildCtx->m_rayResultFn);
		}
		virtual std::string_view GetTypeName() const { return "Ants.BuildWithItems"; }
		virtual RunningState Tick(RunningNodeContext* ctx, BehaviourTreeInstance& bti) const
		{
			SDE_PROF_EVENT();
			if (m_targetAnt == "" || m_targetPosition == "")
			{
				return RunningState::Failed;
			}

			BuildWithItemsContext* buildCtx = static_cast<BuildWithItemsContext*>(ctx);
			if (buildCtx->m_rayFired && !buildCtx->m_resultReady)
			{
				return RunningState::Running;
			}
			if (buildCtx->m_rayFired && buildCtx->m_resultReady)
			{
				static auto entities = Engine::GetSystem<EntitySystem>("Entities");
				EntityHandle targetAnt = bti.m_bb.TryGetEntity(m_targetAnt.c_str());
				AntComponent* targetAntCmp = entities->GetWorld()->GetComponent<AntComponent>(targetAnt);
				glm::vec3 buildPos = buildCtx->m_rayHitPosition;
				for (auto heldEntity : targetAntCmp->m_carriedItems)
				{
					auto heldTrns = entities->GetWorld()->GetComponent<Transform>(heldEntity);
					auto pickupCmp = entities->GetWorld()->GetComponent<AntPickupComponent>(heldEntity);
					auto pickupPhysCmp = entities->GetWorld()->GetComponent<Physics>(heldEntity);
					if (pickupPhysCmp)	// make it a static object
					{
						pickupPhysCmp->SetStatic(true);
						pickupPhysCmp->Rebuild();
					}
					entities->GetWorld()->RemoveComponent<AntPickupComponent>(heldEntity);	// can't be picked up
					auto pickupTags = entities->GetWorld()->GetComponent<Tags>(heldEntity);
					if (pickupTags)
					{
						pickupTags->ClearTags();
						pickupTags->AddTag("BuiltRockInstance");
					}

					// back to world space
					glm::vec3 wsPos, wsScale;
					glm::quat wsOrientation;
					heldTrns->GetWorldSpaceTransform(wsPos, wsOrientation, wsScale);
					heldTrns->SetParent(EntityHandle());
					heldTrns->SetScale(wsScale);
					heldTrns->SetOrientation(wsOrientation);
					heldTrns->SetPosition(buildPos + glm::vec3(Core::Random::GetFloat(-1.0f, 1.0f), 0.0f, Core::Random::GetFloat(-1.0f, 1.0f)));
					buildPos = buildPos + glm::vec3(0.0f, 3.0f, 0.0f);
				}
				targetAntCmp->m_carriedItems.clear();
				return RunningState::Success;
			}
			return RunningState::Failed;
		}

		std::string m_targetAnt;
		std::string m_targetPosition;
	};
	SERIALISE_BEGIN_WITH_PARENT(BuildWithItems, Node)
		SERIALISE_PROPERTY("TargetAnt", m_targetAnt)
		SERIALISE_PROPERTY("TargetPosition", m_targetPosition)
	SERIALISE_END();
}

void AntsSystem::DropAllItems(const EntityHandle& ant)
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto ac = entities->GetWorld()->GetComponent<AntComponent>(ant);
	auto antTrans = entities->GetWorld()->GetComponent<Transform>(ant);
	if (ac && antTrans)
	{
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
		ac->m_carriedItems.clear();
	}
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
	behSys->RegisterNodeType("Ants.FindDropoffPosition", []() {
		return std::make_unique<Behaviours::FindDropoffPosition>();
	});
	behSys->RegisterNodeType("Ants.DropItems", []() {
		return std::make_unique<Behaviours::DropItems>();
	});
	behSys->RegisterNodeType("Ants.BuildWithItems", []() {
		return std::make_unique<Behaviours::BuildWithItems>();
	});
	behSys->RegisterNodeType("Ants.FindBuildPosition", []() {
		return std::make_unique<Behaviours::FindBuildPosition>();
	});

	return true;
}

void AntsSystem::KillAnt(const EntityHandle& e)
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto behSys = Engine::GetSystem<Behaviours::BehaviourTreeSystem>("Behaviours");
	DropAllItems(e);
	auto ac = entities->GetWorld()->GetComponent<AntComponent>(e);
	if (ac)
	{
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
	auto nestEntity = entities->GetFirstEntityWithTag("Nest");
	auto queenEntity = entities->GetFirstEntityWithTag("AntQueen");
	entities->GetWorld()->ForEachComponent<AntComponent>([&](AntComponent& ac, EntityHandle e) {
		if (ac.m_treeInstance == nullptr && ac.m_behaviourTree.size() > 0)
		{
			ac.m_treeInstance = behSys->CreateTreeInstance(ac.m_behaviourTree);
			if (ac.m_treeInstance)
			{
				ac.m_treeInstance->m_bb.SetEntity("@AntQueenEntity", queenEntity);
				ac.m_treeInstance->m_bb.SetEntity("@OwnerEntity", e);
				ac.m_treeInstance->m_bb.SetEntity("@NestEntity", nestEntity);
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