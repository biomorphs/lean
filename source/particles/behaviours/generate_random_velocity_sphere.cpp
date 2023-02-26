#include "generate_random_velocity_sphere.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	void GenerateRandomVelocitySphere::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Min. Magnitude", m_minMagnitude, [this](float val) {
			m_minMagnitude = val;
		});
		v.Inspect("Max. Magnitude", m_maxMagnitude, [this](float val) {
			m_maxMagnitude = val;
		});
	}

	void GenerateRandomVelocitySphere::Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		SDE_PROF_EVENT();

		float range = glm::abs(m_minMagnitude - m_maxMagnitude);
		glm::mat4 emitterTransform = glm::toMat4(orientation);
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			float theta = ((float)rand() / (float)RAND_MAX) * glm::pi<float>() * 2.0f;
			float phi = ((float)rand() / (float)RAND_MAX) * glm::pi<float>() * 2.0f;
			float r = m_minMagnitude + ((float)rand() / (float)RAND_MAX) * range;
			__declspec(align(16)) glm::vec4 newVel(r * glm::cos(theta) * glm::sin(phi), r * glm::sin(theta) * glm::sin(phi), r * glm::cos(phi), 0.0f);
			__m128 velVec = _mm_load_ps(glm::value_ptr(newVel * emitterTransform));
			_mm_stream_ps((float*)&container.Velocities().GetValue(i), velVec);
		}
		_mm_sfence();
	}

	SERIALISE_BEGIN_WITH_PARENT(GenerateRandomVelocitySphere, GeneratorBehaviour)
	{
		SERIALISE_PROPERTY("BoundsMin", m_minMagnitude);
		SERIALISE_PROPERTY("BoundsMax", m_maxMagnitude);
	}
	SERIALISE_END()
}