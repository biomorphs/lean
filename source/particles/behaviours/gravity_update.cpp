#include "gravity_update.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "engine/components/component_environment_settings.h"
#include "engine/physics_system.h"
#include "engine/system_manager.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(GravityUpdate, UpdateBehaviour)
	{
	}
	SERIALISE_END();

	void GravityUpdate::Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();

		// get global gravity value from world
		static auto physics = Engine::GetSystem<Engine::PhysicsSystem>("Physics");
		glm::vec4 gravity = glm::vec4(physics->GetGlobalGravity(),0.0f);

		__declspec(align(16)) const glm::vec4 c_deltaTime((float)deltaTime);
		const __m128 c_gravity = _mm_load_ps(glm::value_ptr(gravity));
		const __m128 c_deltaVec = _mm_load_ps(glm::value_ptr(c_deltaTime));
		const __m128 c_gravMulDelta = _mm_mul_ps(c_gravity, c_deltaVec);

		const uint32_t endIndex = container.AliveParticles();
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			__m128& v = container.Velocities().GetValue(i);
			v = _mm_add_ps(v, c_gravMulDelta);
		}
		_mm_sfence();
	}
}