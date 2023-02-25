#include "generate_spawn_emitter.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"
#include "particles/particle_system.h"
#include "engine/system_manager.h"
#include "core/profiler.h"

namespace Particles
{
	void GenerateSpawnEmitter::Inspect(EditorValueInspector& v)
	{
		v.InspectFilePath("Emitter to spawn", "emit", m_emitterFile, [this](std::string_view val) {
			m_emitterFile = val;
		});
	}

	void GenerateSpawnEmitter::Generate(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		SDE_PROF_EVENT();
		auto particles = Engine::GetSystem<ParticleSystem>("Particles");
		glm::mat4 emitterTransform = glm::translate(emitterPos) * glm::toMat4(orientation);
		__declspec(align(16)) glm::vec4 particlePos;
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			// needs to go After position is set!
			_mm_store_ps(glm::value_ptr(particlePos), container.Positions().GetValue(i));
			uint64_t newEmitter = particles->StartEmitter(m_emitterFile, glm::vec3(particlePos));
			container.EmitterIDs().GetValue(i) = newEmitter;
		}
		_mm_sfence();
	}

	SERIALISE_BEGIN_WITH_PARENT(GenerateSpawnEmitter, GeneratorBehaviour)
	{
		SERIALISE_PROPERTY("EmitterFile", m_emitterFile);
	}
	SERIALISE_END()
}