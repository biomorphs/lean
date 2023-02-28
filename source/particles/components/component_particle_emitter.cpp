#include "component_particle_emitter.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"

COMPONENT_SCRIPTS(ComponentParticleEmitter,
	"SetEmitter", &ComponentParticleEmitter::SetEmitter,
	"RestartEffect", &ComponentParticleEmitter::RestartEffect
)

SERIALISE_BEGIN(ComponentParticleEmitter)
	SERIALISE_PROPERTY("EmitterFile", m_emitterFile)
	if (op == Engine::SerialiseType::Read)
	{
		m_emitterDirty = true;
		m_playingEmitterID = -1;
	}
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(ComponentParticleEmitter)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		static EntityHandle setParentEntity;
		auto& p = *static_cast<ComponentParticleEmitter::StorageType&>(cs).Find(e);
		i.InspectFilePath("Emitter File", "emit", p.GetEmitter(), [&](std::string newfile) {
			p.SetEmitter(newfile);
		});
		if (gui->Button("Restart"))
		{
			p.RestartEffect();
		}
	};
	return fn;
}