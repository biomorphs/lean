#include "euler_position_update.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(EulerPositionUpdater, UpdateBehaviour)
	{
	}
	SERIALISE_END();

	void EulerPositionUpdater::Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		__declspec(align(16)) const glm::vec4 c_deltaTime((float)deltaTime);
		const __m128 c_deltaVec = _mm_load_ps(glm::value_ptr(c_deltaTime));

		const uint32_t endIndex = container.AliveParticles();
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			__m128& p = container.Positions().GetValue(i);
			const __m128& v = container.Velocities().GetValue(i);
			const __m128 vMulDelta = _mm_mul_ps(v, c_deltaVec);
			p = _mm_add_ps(p, vMulDelta);
		}
		_mm_sfence();
	}
}