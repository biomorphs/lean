#pragma once

#include "entity/component.h"
#include "core/glm_headers.h"
#include "engine/physics_handle.h"

namespace physx
{
	class PxRigidActor;
}

namespace Engine
{
	class DebugGuiSystem;
	class DebugRender;
}

struct PlaneCollider {
	glm::vec3 m_normal;
	glm::vec3 m_origin;
	SERIALISED_CLASS();
};

struct SphereCollider {
	glm::vec3 m_origin;
	float m_radius;
	SERIALISED_CLASS();
};

struct BoxCollider {
	glm::vec3 m_origin;
	glm::vec3 m_dimensions;
	SERIALISED_CLASS();
};

struct CapsuleCollider {
	glm::vec3 m_origin;
	glm::vec3 m_pitchYawRoll;
	float m_radius;
	float m_halfHeight;
	SERIALISED_CLASS();
};

class Physics
{
public:
	Physics();
	~Physics();
	Physics(Physics&&) = default;
	Physics& operator=(Physics&&) = default;
	Physics(const Physics&) = delete;
	Physics& operator=(const Physics&) = delete;
	
	COMPONENT(Physics);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui, Engine::DebugRender& render, class World& w);

	void AddForce(glm::vec3 force);

	bool IsStatic() { return m_isStatic; }
	void SetStatic(bool s) { m_isStatic = s; }

	bool IsKinematic() { return m_isKinematic; }
	void SetKinematic(bool k) { m_isKinematic = k; }

	float GetDensity() { return m_density; }
	void SetDensity(float d) { m_density = d; }

	float GetStaticFriction() { return m_staticFriction; }
	void SetStaticFriction(float s) { m_staticFriction = s; }

	float GetDynamicFriction() { return m_dynamicFriction; }
	void SetDynamicFriction(float s) { m_dynamicFriction = s; }

	float GetRestitution() { return m_restitution; }
	void SetRestitution(float s) { m_restitution = s; }

	using PlaneColliders = std::vector<PlaneCollider>;
	void AddPlaneCollider(glm::vec3 normal, glm::vec3 origin) { m_planeColliders.push_back({normal,origin}); }
	PlaneColliders& GetPlaneColliders() { return m_planeColliders; }

	using SphereColliders = std::vector<SphereCollider>;
	void AddSphereCollider(glm::vec3 offset, float radius) { m_sphereColliders.push_back({offset, radius}); }
	SphereColliders& GetSphereColliders() { return m_sphereColliders; }

	using BoxColliders = std::vector<BoxCollider>;
	void AddBoxCollider(glm::vec3 offset, glm::vec3 dim) { m_boxColliders.push_back({ offset, dim }); }
	BoxColliders& GetBoxColliders() { return m_boxColliders; }

	using CapsuleColliders = std::vector<CapsuleCollider>;
	void AddCapsuleCollider(glm::vec3 offset, glm::vec3 pitchYawRoll, float radius, float halfHeight)
	{ 
		m_capsuleColliders.push_back({offset, pitchYawRoll, radius, halfHeight});
	}
	CapsuleColliders& GetCapsuleColliders() { return m_capsuleColliders; }

	void Rebuild() { m_needsRebuild = true; }
	bool NeedsRebuild() { return m_needsRebuild; }
	void SetNeedsRebuild(bool b) { m_needsRebuild = b; }

	Engine::PhysicsHandle<physx::PxRigidActor>& GetActor() { return m_actor; }
	void SetActor(Engine::PhysicsHandle<physx::PxRigidActor>&& a) { m_actor = std::move(a); }

private:
	bool m_needsRebuild = false;
	bool m_isStatic = false;
	bool m_isKinematic = false;		

	// Material parameters
	float m_density = 1.0f;
	float m_staticFriction = 0.5f;
	float m_dynamicFriction = 0.5f;
	float m_restitution = 0.6f;

	PlaneColliders m_planeColliders;
	SphereColliders m_sphereColliders;
	BoxColliders m_boxColliders;
	CapsuleColliders m_capsuleColliders;

	Engine::PhysicsHandle<physx::PxRigidActor> m_actor;
};