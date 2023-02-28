#pragma once

#include "entity/component.h"
#include "entity/component_handle.h"
#include "core/glm_headers.h"

class ComponentParticleEmitter
{
public:
	COMPONENT(ComponentParticleEmitter);
	COMPONENT_INSPECTOR();

	void SetEmitter(std::string_view emitterPath) {	m_emitterFile = emitterPath; m_emitterDirty = true; }
	std::string_view GetEmitter() { return m_emitterFile; }

	void SetPlayingEmitterID(uint32_t id) { m_playingEmitterID = id; m_emitterDirty = false; }
	uint32_t GetPlayingEmitterID() { return m_playingEmitterID; }

	void RestartEffect() { m_emitterDirty = true; }	// may be useful?
	bool NeedsRestart() { return m_emitterDirty; }

private:
	std::string m_emitterFile;
	uint32_t m_playingEmitterID = -1;
	bool m_emitterDirty = true;	// determine if emitter needs to be (re)started
};