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
		v.Inspect("Min. Theta", m_minTheta, [this](float val) {
			m_minTheta = val;
		});
		v.Inspect("Max. Theta", m_maxTheta, [this](float val) {
			m_maxTheta = val;
		});
		v.Inspect("Min. Phi", m_minPhi, [this](float val) {
			m_minPhi = val;
		});
		v.Inspect("Max. Phi", m_maxPhi, [this](float val) {
			m_maxPhi = val;
		});
	}

	void GenerateRandomVelocitySphere::Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		SDE_PROF_EVENT();

		float range = glm::abs(m_minMagnitude - m_maxMagnitude);
		glm::mat4 emitterTransform = glm::toMat4(orientation);
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			float theta = m_minTheta + ((float)rand() / (float)RAND_MAX) * glm::abs(m_maxTheta - m_minTheta);
			float phi = m_minPhi + ((float)rand() / (float)RAND_MAX) * glm::abs(m_maxPhi - m_minPhi);
			float r = m_minMagnitude + ((float)rand() / (float)RAND_MAX) * range;
			__declspec(align(16)) glm::vec4 newVel(r * glm::cos(theta) * glm::sin(phi), r * glm::sin(theta) * glm::sin(phi), r * glm::cos(phi), 0.0f);
			__m128 velVec = _mm_load_ps(glm::value_ptr(newVel * emitterTransform));
			container.Velocities().GetValue(i) = velVec;
		}
	}

	SERIALISE_BEGIN_WITH_PARENT(GenerateRandomVelocitySphere, GeneratorBehaviour)
	{
		SERIALISE_PROPERTY("BoundsMin", m_minMagnitude);
		SERIALISE_PROPERTY("BoundsMax", m_maxMagnitude);
		SERIALISE_PROPERTY("MinTheta", m_minTheta);
		SERIALISE_PROPERTY("MaxTheta", m_maxTheta);
		SERIALISE_PROPERTY("MinPhi", m_minPhi);
		SERIALISE_PROPERTY("MaxPhi", m_maxPhi);
	}
	SERIALISE_END()
}