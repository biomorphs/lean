#include "update_spawn_emitter.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "particles/particle_system.h"
#include "engine/system_manager.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(UpdateSpawnEmitter, UpdateBehaviour)
	{
		SERIALISE_PROPERTY("EmitterFile", m_emitterFile);
		SERIALISE_PROPERTY("AttachToParticle", m_attachToParticle);
		SERIALISE_PROPERTY("EmitterCount", m_burstCount);
		SERIALISE_PROPERTY("SpawnFrequency", m_frequency);
		SERIALISE_PROPERTY("SpawnStartAge", m_spawnStartAge);
		SERIALISE_PROPERTY("SpawnEndAge", m_spawnEndAge);
	}
	SERIALISE_END();

	void UpdateSpawnEmitter::Inspect(EditorValueInspector& v)
	{
		v.InspectFilePath("Emitter", "emit", m_emitterFile, [this](std::string_view val) {
			m_emitterFile = val;
		});
		v.Inspect("Attach emitter to particle", m_attachToParticle, [this](bool v) {
			m_attachToParticle = v;
		});
		v.Inspect("Emitters to spawn", m_burstCount, [this](int v) {
			m_burstCount = v;
		});
		v.Inspect("Spawn frequency", m_frequency, [this](float v) {
			m_frequency = v;
		});
		v.Inspect("Spawn start age", m_spawnStartAge, [this](float v) {
			m_spawnStartAge = v;
		});
		v.Inspect("Spawn end age", m_spawnEndAge, [this](float v) {
			m_spawnEndAge = v;
		});
	}

	void UpdateSpawnEmitter::Update(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		static auto particles = Engine::GetSystem<ParticleSystem>("Particles");
		const uint32_t endIndex = container.AliveParticles(); 
		__declspec(align(16)) glm::vec4 particlePos;
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			float particleAge = emitterAge - container.SpawnTimes().GetValue(i);
			if (particleAge > m_spawnStartAge && (particleAge <= m_spawnEndAge || m_spawnEndAge < 0))
			{
				float v = fmod(particleAge, m_frequency);
				if (v > -0.016f && v <= 0.016f)
				{
					_mm_store_ps(glm::value_ptr(particlePos), container.Positions().GetValue(i));
					for (int em = 0; em < m_burstCount; ++em)
					{
						uint32_t newEmitter = particles->StartEmitter(m_emitterFile, glm::vec3(particlePos));
						if (m_attachToParticle)
						{
							container.EmitterIDs().GetValue(i) = newEmitter;
						}
					}
				}
			}
		}
	}
}