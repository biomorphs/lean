#include "particle_system.h"
#include "emitter_instance.h"
#include "core/file_io.h"
#include "engine/job_system.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/components/component_transform.h"
#include "entity/entity_system.h"
#include "components/component_particle_emitter.h"
#include "core/timer.h"

namespace Particles
{
	ParticleSystem::ParticleSystem()
	{
		m_activeEmitters.reserve(m_maxEmitters);
		m_activeEmitterIDToIndex.reserve(m_maxEmitters);
		m_emittersToStart.reserve(m_maxEmitters);
	}

	ParticleSystem::~ParticleSystem()
	{
	}

	bool ParticleSystem::PostInit()
	{
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		entities->RegisterComponentType<ComponentParticleEmitter>();
		entities->RegisterInspector<ComponentParticleEmitter>(ComponentParticleEmitter::MakeInspector());
		return true;
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

	bool ParticleSystem::SetEmitterTransform(EmitterID emitterID, glm::vec3 pos, glm::quat rot)
	{
		const auto foundInstance = m_activeEmitterIDToIndex.find(emitterID);
		if (foundInstance != m_activeEmitterIDToIndex.end())
		{
			const uint32_t index = foundInstance->second;
			m_activeEmitters[index].m_instance->m_position = pos;
			m_activeEmitters[index].m_instance->m_orientation = rot;
			return true;
		}
		return false;
	}

	void ParticleSystem::StopEmitter(EmitterID emitterID)
	{
		Core::ScopedMutex lock(m_stopEmittersMutex);
		m_emittersToStop.push_back(emitterID);
	}

	ParticleSystem::EmitterID ParticleSystem::StartEmitter(std::string_view filename, glm::vec3 pos, glm::quat rot)
	{
		SDE_PROF_EVENT();

		std::string filenameStr(filename);
		EmitterDescriptor* loadedEmitter = nullptr;
		{
			Core::ScopedMutex lock(m_loadedEmittersMutex);
			auto foundEmitter = m_loadedEmitters.find(filenameStr);
			if (foundEmitter != m_loadedEmitters.end())
			{
				loadedEmitter = (foundEmitter->second.get());
			}
			else
			{
				auto newEmitter = std::make_unique<EmitterDescriptor>();
				if (LoadEmitter(filename, *newEmitter))
				{
					loadedEmitter = newEmitter.get();
					m_loadedEmitters[filenameStr] = std::move(newEmitter);
				}
			}
		}
		if (loadedEmitter)
		{
			auto newInstance = new EmitterInstance();
			newInstance->m_emitter = loadedEmitter;
			newInstance->m_particles.Create(newInstance->m_emitter->GetMaxParticles());
			newInstance->m_position = pos;
			newInstance->m_orientation = rot;
			newInstance->m_timeActive = 0.0;

			ActiveEmitter activeEmitter;
			activeEmitter.m_id = m_nextEmitterId++;
			activeEmitter.m_instance = newInstance;
			{
				Core::ScopedMutex lock(m_startEmittersMutex);
				m_emittersToStart.emplace_back(activeEmitter);
			}
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

		// Lifetime before spawning anything
		for(const auto& it : emitterDesc->GetLifetimeBehaviours())
		{
			if (it->ShouldStop(emitterAge, timeDelta, particles))
			{
				StopEmitter(em.m_id);
				return;
			}
		}

		// Emission pass, calculate emission count, generate particles
		uint32_t emissionCount = 0;
		for (const auto& it : emitterDesc->GetEmissionBehaviours())
		{
			emissionCount += it->Emit(emitterAge, timeDelta);
		}
		emissionCount = glm::min(emissionCount, particles.MaxParticles() - particles.AliveParticles());
		if (emissionCount > 0)
		{
			const uint32_t startIndex = particles.Wake(emissionCount, emitterAge);
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

		Core::ScopedMutex lock(m_invalidatedEmitterMutex);
		m_invalidatedEmitters.push_back(std::string(filename));
	}

	void ParticleSystem::ReloadInvalidatedEmitters()
	{
		SDE_PROF_EVENT();

		Core::ScopedMutex lock(m_invalidatedEmitterMutex);
		Core::ScopedMutex lock2(m_loadedEmittersMutex);
		for (const auto& path : m_invalidatedEmitters)
		{
			auto foundEmitter = m_loadedEmitters.find(path);
			if (foundEmitter != m_loadedEmitters.end())
			{
				foundEmitter->second->Reset();
				LoadEmitter(path, *foundEmitter->second);
			}
		}
		m_invalidatedEmitters.clear();
	}

	void ParticleSystem::ShowStats()
	{
		SDE_PROF_EVENT();
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		dbgGui->BeginWindow(m_showStats, "Particle Stats");
		std::string statText = "Active Emitters: " + std::to_string(m_activeEmitters.size());
		dbgGui->Text(statText.c_str());

		uint64_t particleCount = 0;
		for (const auto& it : m_activeEmitters)
		{
			particleCount += it.m_instance->m_particles.AliveParticles();
		}
		statText = "Active Particles: " + std::to_string(particleCount);
		dbgGui->Text(statText.c_str());

		float updateMs = float(m_lastUpdateTime * 1000.0);
		float renderMs = float(m_lastRenderTime * 1000.0);
		statText = "Update Time(ms): " + std::to_string(updateMs);
		dbgGui->Text(statText.c_str());
		statText = "Render Time(ms): " + std::to_string(renderMs);
		dbgGui->Text(statText.c_str());

		dbgGui->EndWindow();
	}

	void ParticleSystem::StartNewEmitters()
	{
		SDE_PROF_EVENT();
		Core::ScopedMutex lock(m_startEmittersMutex);
		for (auto& toAdd : m_emittersToStart)
		{
			const EmitterID newId = toAdd.m_id;
			m_activeEmitters.emplace_back(std::move(toAdd));
			m_activeEmitterIDToIndex[newId] = m_activeEmitters.size() - 1;
			assert(m_activeEmitterIDToIndex.size() == m_activeEmitters.size());
		}
		m_emittersToStart.clear();
	}

	void ParticleSystem::UpdateEmitters(float timeDelta)
	{
		SDE_PROF_EVENT();
		Core::ScopedTimer timeUpdate(m_lastUpdateTime);
		auto jobs = Engine::GetSystem<Engine::JobSystem>("Jobs");
		jobs->ForEachAsync(0, m_activeEmitters.size(), 1, 32, [this, timeDelta](int32_t i) {
			UpdateActiveInstance(m_activeEmitters[i], 0.016f);
		});
	}

	void ParticleSystem::DoStopEmitter(EmitterInstance& i)
	{
		if (i.m_emitter->GetOwnsChildEmitters())
		{
			int ownedEmitters = i.m_particles.EmitterIDs().AliveCount();
			for (int e = 0; e < ownedEmitters; ++e)
			{
				int emitterId = i.m_particles.EmitterIDs().GetValue(e);
				if (emitterId != -1)
				{
					StopEmitter(emitterId);
				}
			}
		}
	}

	void ParticleSystem::StopEmitters()
	{
		SDE_PROF_EVENT();
		std::vector<EmitterID> emittersToStop;
		{
			Core::ScopedMutex lock(m_stopEmittersMutex);
			emittersToStop = std::move(m_emittersToStop);
		}
		for (const EmitterID toStopID : emittersToStop)
		{
			auto foundIt = m_activeEmitterIDToIndex.find(toStopID);
			if (foundIt != m_activeEmitterIDToIndex.end())
			{
				const uint32_t emitterIndex = foundIt->second;

				DoStopEmitter(*m_activeEmitters[emitterIndex].m_instance);
				delete m_activeEmitters[emitterIndex].m_instance;	// pool these
				m_activeEmitters[emitterIndex].m_instance = nullptr;

				// swap back, update mappings
				m_activeEmitterIDToIndex.erase(toStopID);	// remove  old id
				if (emitterIndex != m_activeEmitters.size() - 1)
				{
					m_activeEmitters[emitterIndex] = std::move(m_activeEmitters[m_activeEmitters.size() - 1]);
					m_activeEmitterIDToIndex[m_activeEmitters[emitterIndex].m_id] = emitterIndex;
				}
				m_activeEmitters.resize(m_activeEmitters.size() - 1);
				assert(m_activeEmitterIDToIndex.size() == m_activeEmitters.size());
			}
		}
	}

	void ParticleSystem::RenderEmitters(float timeDelta	)
	{
		SDE_PROF_EVENT();
		Core::ScopedTimer timeUpdate(m_lastRenderTime);
		for (const auto& em : m_activeEmitters)
		{
			for (const auto& render : em.m_instance->m_emitter->GetRenderers())
			{
				render->Draw(em.m_instance->m_position, em.m_instance->m_orientation, em.m_instance->m_timeActive, timeDelta, em.m_instance->m_particles);
			}
		}
	}

	void ParticleSystem::UpdateEmitterComponents()
	{
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		auto world = entities->GetWorld();
		static auto emitterIterator = world->MakeIterator<ComponentParticleEmitter, Transform>();
		emitterIterator.ForEach([this](ComponentParticleEmitter & em, Transform & t, EntityHandle owner) {
			uint32_t playingID = em.GetPlayingEmitterID();
			glm::quat orientation;
			glm::vec3 position, scale;
			t.GetWorldSpaceTransform(position, orientation, scale);
			if (em.NeedsRestart())
			{				
				if (playingID != -1)
				{
					StopEmitter(playingID);
					playingID = -1;
				}
				if (em.GetEmitter().size() > 0)
				{
					playingID = StartEmitter(em.GetEmitter(), position, orientation);
				}
				em.SetPlayingEmitterID(playingID);
			}
			else if (playingID != -1)
			{
				if (!SetEmitterTransform(playingID, position, orientation))
				{
					em.SetPlayingEmitterID(-1);
				}
			}
		});
	}

	bool ParticleSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();
		
		ReloadInvalidatedEmitters();
		StartNewEmitters();
		if (m_updateEmitters)
		{
			UpdateEmitters(timeDelta);
		}
		StopEmitters();
		if (m_renderEmitters)
		{
			RenderEmitters(timeDelta);
		}
		UpdateEmitterComponents();

		if (m_showStats)
		{
			ShowStats();
		}
		return true;
	}
}