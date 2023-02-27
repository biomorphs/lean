#include "generate_random_position.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	void GenerateRandomPosition::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Bounds Min", m_boundsMin, [this](glm::vec3 val) {
			m_boundsMin = val;
		});
		v.Inspect("Bounds Max", m_boundsMax, [this](glm::vec3 val) {
			m_boundsMax = val;
		});
	}

	void GenerateRandomPosition::Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		SDE_PROF_EVENT();
		glm::mat4 emitterTransform = glm::translate(emitterPos) * glm::toMat4(orientation);
		glm::vec4 range = glm::abs(glm::vec4(m_boundsMin - m_boundsMax, 0.0f));
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			float vX = ((float)rand() / (float)RAND_MAX);
			float vY = ((float)rand() / (float)RAND_MAX);
			float vZ = ((float)rand() / (float)RAND_MAX);

			__declspec(align(16)) glm::vec4 newPos(m_boundsMin.x + (vX * range.x), m_boundsMin.y + (vY * range.y), m_boundsMin.z + (vZ * range.z), 1.0f);
			newPos = emitterTransform * newPos;
			__m128 posVec = _mm_load_ps(glm::value_ptr(newPos));
			container.Positions().GetValue(i) = posVec;
		}
	}

	SERIALISE_BEGIN_WITH_PARENT(GenerateRandomPosition, GeneratorBehaviour)
	{
		SERIALISE_PROPERTY("BoundsMin", m_boundsMin);
		SERIALISE_PROPERTY("BoundsMax", m_boundsMax);
	}
	SERIALISE_END()
}