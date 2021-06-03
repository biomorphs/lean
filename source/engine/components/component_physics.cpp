#include "component_physics.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_render.h"
#include "entity/entity_handle.h"
#include "component_transform.h"
#include "entity/world.h"
#include <PxPhysicsAPI.h>

COMPONENT_SCRIPTS(Physics,
	"AddForce", &Physics::AddForce,
	"IsStatic", &Physics::IsStatic,
	"SetStatic", &Physics::SetStatic,
	"SetKinematic", &Physics::SetKinematic,
	"IsKinematic", &Physics::IsKinematic,
	"SetDensity", &Physics::SetDensity,
	"GetStaticFriction", &Physics::GetStaticFriction,
	"SetStaticFriction", &Physics::SetStaticFriction,
	"SetDynamicFriction", &Physics::SetDynamicFriction,
	"GetDynamicFriction", &Physics::GetDynamicFriction,
	"SetRestitution", &Physics::SetRestitution,
	"GetRestitution", &Physics::GetRestitution,
	"AddPlaneCollider", &Physics::AddPlaneCollider,
	"AddSphereCollider", &Physics::AddSphereCollider,
	"AddBoxCollider", &Physics::AddBoxCollider,
	"Rebuild", &Physics::Rebuild
)

Physics::Physics()
{

}

Physics::~Physics()
{
	m_actor = nullptr;
}

void Physics::AddForce(glm::vec3 force)
{
	assert(!IsKinematic() && !IsStatic() && m_actor.Get() != nullptr);
	if (!IsKinematic() && !IsStatic() && m_actor.Get() != nullptr)
	{
		physx::PxVec3 fv(force.x, force.y, force.z);
		static_cast<physx::PxRigidDynamic*>(m_actor.Get())->addForce(fv, physx::PxForceMode::eFORCE);
	}
}

COMPONENT_INSPECTOR_IMPL(Physics, Engine::DebugGuiSystem& gui, Engine::DebugRender& render, World& world)
{
	auto fn = [&gui, &render, &world](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& p = *static_cast<Physics::StorageType&>(cs).Find(e);
		p.SetStatic(gui.Checkbox("Static", p.IsStatic()));
		if (!p.IsStatic())
		{
			p.SetKinematic(gui.Checkbox("Kinematic", p.IsKinematic()));
		}
		p.SetDensity(gui.DragFloat("Density", p.GetDensity(), 0.01f, 0.0f, 100000.0f));
		p.SetStaticFriction(gui.DragFloat("Friction (Static)", p.GetStaticFriction(), 0.01f, 0.0f, 10.0f));
		p.SetDynamicFriction(gui.DragFloat("Friction (Dynamic)", p.GetDynamicFriction(), 0.01f, 0.0f, 10.0f));
		p.SetRestitution(gui.DragFloat("Restitution", p.GetRestitution(), 0.01f, 0.0f, 10.0f));
		if (gui.TreeNode("Colliders", true))
		{
			char text[256] = "";
			for (auto& it : p.GetPlaneColliders())
			{
				sprintf(text, "Plane %d", (int)(&it - p.GetPlaneColliders().data()));
				if (gui.TreeNode(text))
				{
					std::get<0>(it) = gui.DragVector("Normal", std::get<0>(it), 0.01f, -1.0f, 1.0f);
					std::get<1>(it) = gui.DragVector("Origin", std::get<1>(it), 0.01f, -1.0f, 1.0f);
					gui.TreePop();
					render.DrawLine(std::get<1>(it), std::get<1>(it) + std::get<0>(it), { 0.0f,1.0f,1.0f });
				}
			}
			for (auto& it : p.GetSphereColliders())
			{
				sprintf(text, "Sphere %d", (int)(&it - p.GetSphereColliders().data()));
				if (gui.TreeNode(text))
				{
					std::get<0>(it) = gui.DragVector("Offset", std::get<0>(it), 0.01f);
					std::get<1>(it) = gui.DragFloat("Radius", std::get<1>(it), 0.01f, 0.0f, 100000.0f);
					gui.TreePop();
					auto transform = world.GetComponent<Transform>(e);
					if (transform)
					{
						auto bMin = std::get<0>(it) - std::get<1>(it);
						auto bMax = std::get<0>(it) + std::get<1>(it);
						// ignore scale since we dont pass it to physx
						glm::mat4 matrix = glm::translate(glm::identity<glm::mat4>(), transform->GetPosition());
						matrix = matrix * glm::toMat4(transform->GetOrientation());
						render.DrawBox(bMin, bMax, { 0.0f,1.0f,1.0f,1.0f }, matrix);
					}
				}
			}
			for (auto& it : p.GetBoxColliders())
			{
				sprintf(text, "Box %d", (int)(&it - p.GetBoxColliders().data()));
				if (gui.TreeNode(text))
				{
					std::get<0>(it) = gui.DragVector("Offset", std::get<0>(it), 0.01f);
					std::get<1>(it) = gui.DragVector("Dimensions", std::get<1>(it), 0.01f, 0.0f, 100000.0f);
					gui.TreePop();
					auto transform = world.GetComponent<Transform>(e);
					if (transform && transform->GetScale().length() != 0.0f)
					{
						auto bMin = std::get<0>(it) - std::get<1>(it) * 0.5f;
						auto bMax = std::get<0>(it) + std::get<1>(it) * 0.5f;
						// ignore scale since we dont pass it to physx
						glm::mat4 matrix = glm::translate(glm::identity<glm::mat4>(), transform->GetPosition());
						matrix = matrix * glm::toMat4(transform->GetOrientation());
						render.DrawBox(bMin, bMax, { 0.0f,1.0f,1.0f,1.0f }, matrix);
					}
				}
			}
			if (gui.Button("+ Plane"))
			{
				p.AddPlaneCollider({ 0,1,0 }, { 0,0,0 });
			}
			gui.SameLine();
			if (gui.Button("+ Sphere"))
			{
				p.AddSphereCollider({ 0,0,0 }, 1.0f);
			}
			gui.SameLine();
			if (gui.Button("+ Box"))
			{
				p.AddBoxCollider({ 0,0,0 }, { 1,1,1 });
			}
			gui.TreePop();
			if (gui.Button("Rebuild"))
			{
				p.Rebuild();
			}
		}
	};
	return fn;
}