#include "generate_random_lifetime.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "core/profiler.h"

namespace Particles
{
	void GenerateRandomLifetime::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Min Particle Life", m_minLifetime, [this](float val) {
			m_minLifetime = val;
		});
		v.Inspect("Max Particle LIfe", m_maxLifetime, [this](float val) {
			m_maxLifetime = val;
		});
	}

	void GenerateRandomLifetime::Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		SDE_PROF_EVENT();
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			float v = ((float)rand() / (float)RAND_MAX);
			float particleLife = m_minLifetime + (m_maxLifetime- m_minLifetime) * v;
			container.Lifetimes().GetValue(i) = particleLife;
		}
		_mm_sfence();
	}

	SERIALISE_BEGIN_WITH_PARENT(GenerateRandomLifetime, GeneratorBehaviour)
	{
		SERIALISE_PROPERTY("MinLife", m_minLifetime);
		SERIALISE_PROPERTY("MaxLife", m_maxLifetime);
	}
	SERIALISE_END()
}