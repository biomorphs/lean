#include "particle_system.h"
#include "emitter_instance.h"
#include "core/file_io.h"
#include "engine/job_system.h"
#include "engine/system_manager.h"

namespace Particles
{
	ParticleSystem::ParticleSystem()
	{
	}

	ParticleSystem::~ParticleSystem()
	{
	}

	bool ParticleSystem::LoadEmitter(std::string_view path, EmitterDescriptor& result)
	{
		SDE_PROF_EVENT();
		
		std::string fileText;
		if (!Core::LoadTextFromFile(path.data(), fileText))
		{
			SDE_LOG("Failed to load emitter '%s'", path.data());
			return false;
		}
		nlohmann::json emitterJson;
		{
			SDE_PROF_EVENT("ParseJson");
			emitterJson = nlohmann::json::parse(fileText);
		}

		auto newEmitter = std::make_unique<EmitterDescriptor>();
		result.Serialise(emitterJson, Engine::SerialiseType::Read);

		return true;
	}

	uint64_t ParticleSystem::StartEmitter(std::string_view filename, glm::vec3 pos, glm::quat rot)
	{
		SDE_PROF_EVENT();

		std::string filenameStr(filename);
		auto foundEmitter = m_loadedEmitters.find(filenameStr);
		if (foundEmitter == m_loadedEmitters.end())
		{
			auto newEmitter = std::make_unique<EmitterDescriptor>();
			if (LoadEmitter(filename, *newEmitter))
			{
				m_loadedEmitters[filenameStr] = std::move(newEmitter);
				foundEmitter = m_loadedEmitters.find(filenameStr);
			}
		}

		if (foundEmitter != m_loadedEmitters.end())
		{
			auto newInstance = new EmitterInstance();
			newInstance->m_emitter = (*foundEmitter).second.get();
			newInstance->m_particles.Create(newInstance->m_emitter->GetMaxParticles());
			newInstance->m_position = pos;
			newInstance->m_orientation = rot;
			newInstance->m_timeActive = 0.0;

			ActiveEmitter activeEmitter;
			activeEmitter.m_id = m_nextEmitterId++;
			activeEmitter.m_instance = newInstance;
			m_activeEmitters.emplace_back(activeEmitter);

			return activeEmitter.m_id;
		}

		return -1;
	}

	void ParticleSystem::UpdateActiveInstance(ActiveEmitter& em, float timeDelta)
	{
		SDE_PROF_EVENT();

		auto& emitterDesc = em.m_instance->m_emitter;
		auto& particles = em.m_instance->m_particles;
		double emitterAge = em.m_instance->m_timeActive;

		// Emission pass, calculate emission count, generate particles
		uint32_t emissionCount = 0;
		for (const auto& it : emitterDesc->GetEmissionBehaviours())
		{
			emissionCount += it->Emit(emitterAge, timeDelta);
		}
		emissionCount = glm::min(emissionCount, particles.MaxParticles() - particles.AliveParticles());
		if (emissionCount > 0)
		{
			const uint32_t startIndex = particles.Wake(emissionCount);
			const uint32_t endIndex = particles.AliveParticles();
			for (const auto& it : emitterDesc->GetGenerators())
			{
				it->Generate(em.m_instance->m_position, em.m_instance->m_orientation, emitterAge, timeDelta, particles, startIndex, endIndex);
			}
		}

		// Update pass - run on all particles
		for (const auto& it : emitterDesc->GetUpdaters())
		{
			it->Update(em.m_instance->m_position, em.m_instance->m_orientation, emitterAge, timeDelta, particles);		// Particles may be killed during this
		}

		em.m_instance->m_timeActive += timeDelta;
	}

	void ParticleSystem::InvalidateEmitter(std::string_view filename)	// force reload
	{
		SDE_PROF_EVENT();

		auto foundEmitter = m_loadedEmitters.find(std::string(filename));
		if (foundEmitter != m_loadedEmitters.end())
		{
			foundEmitter->second->Reset();
			LoadEmitter(filename, *foundEmitter->second);
		}
	}

	bool ParticleSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		static bool firstRun = true;
		if (firstRun)
		{
			StartEmitter("test.emit");
			firstRun = false;
		}
		if (m_updateEmitters)
		{
			auto jobs = Engine::GetSystem<Engine::JobSystem>("Jobs");
			jobs->ForEachAsync(0, m_activeEmitters.size(), 1, 8, [this, timeDelta](int32_t i) {
				UpdateActiveInstance(m_activeEmitters[i], timeDelta);
			});
		}
		if (m_renderEmitters)
		{
			for (const auto& em : m_activeEmitters)
			{
				for (const auto& render : em.m_instance->m_emitter->GetRenderers())
				{
					render->Draw(em.m_instance->m_timeActive, timeDelta, em.m_instance->m_particles);
				}
			}
		}
		return true;
	}
}