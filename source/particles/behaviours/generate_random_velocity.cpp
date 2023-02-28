#include "generate_random_velocity.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	void GenerateRandomVelocity::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Min. Vale", m_minimum, [this](glm::vec3 val) {
			m_minimum = val;
		});
		v.Inspect("Max. Value", m_maximum, [this](glm::vec3 val) {
			m_maximum = val;
		});
	}

	void GenerateRandomVelocity::Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		SDE_PROF_EVENT();

		glm::vec4 range = glm::abs(glm::vec4(m_minimum - m_maximum, 0.0f));
		glm::mat4 emitterTransform = glm::toMat4(orientation);
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			float vX = ((float)rand() / (float)RAND_MAX);
			float vY = ((float)rand() / (float)RAND_MAX);
			float vZ = ((float)rand() / (float)RAND_MAX);

			__declspec(align(16)) glm::vec4 newVel(m_minimum.x + (vX * range.x), m_minimum.y + (vY * range.y), m_minimum.z + (vZ * range.z), 0.0f);
			__m128 velVec = _mm_load_ps(glm::value_ptr(emitterTransform * newVel));
			container.Velocities().GetValue(i) = velVec;
		}
	}

	SERIALISE_BEGIN_WITH_PARENT(GenerateRandomVelocity, GeneratorBehaviour)
	{
		SERIALISE_PROPERTY("BoundsMin", m_minimum);
		SERIALISE_PROPERTY("BoundsMax", m_maximum);
	}
	SERIALISE_END()
}