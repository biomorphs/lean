#include "ants.h"
#include "core/file_io.h"
#include "entity/entity_system.h"
#include "entity/component.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"
#include "engine/components/component_transform.h"
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
SERIALISE_END()
COMPONENT_INSPECTOR_IMPL(AntComponent)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		static EntityHandle setParentEntity;
		auto& t = *static_cast<AntComponent::StorageType&>(cs).Find(e);
		i.InspectFilePath("Behavior Tree", "beht", t.m_behaviourTree, InspectFn(e, &AntComponent::SetBehaviourTree));
		i.Inspect("Energy", t.m_energy, InspectFn(e, &AntComponent::SetEnergy));
		i.Inspect("Hunger energy threshold", t.m_minEnergyForHunger, InspectFn(e, &AntComponent::SetMinEnergyForHunger));
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
						closestFound = owner;
						closestDistance = distance;
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
				float distance = glm::distance(walkerCmp->GetPosition(), targetCmp->GetPosition());
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
}

bool AntsSystem::PostInit()
{
	SDE_PROF_EVENT();
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	entities->RegisterComponentType<AntFoodComponent>();
	entities->RegisterInspector<AntFoodComponent>(AntFoodComponent::MakeInspector());
	entities->RegisterComponentType<AntComponent>();
	entities->RegisterInspector<AntComponent>(AntComponent::MakeInspector());

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

	return true;
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
	});

	return true;
}