#pragma once 
#include "particles/particle_emission_behaviour.h"
#include "engine/shader_manager.h"
#include "engine/model_manager.h"

namespace Particles
{
	class AttachedLightBehaviour : public RenderBehaviour
	{
	public:
		SERIALISED_CLASS();
		virtual void Draw(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container);
		virtual std::unique_ptr<RenderBehaviour> MakeNew() { return std::make_unique<AttachedLightBehaviour>(); }
		virtual std::string_view GetName() { return "Attached point light"; }
		virtual void Inspect(EditorValueInspector&);
		
		bool m_attachToEmitter = true;
		bool m_attachToParticles = false;
		glm::vec3 m_colour = { 1.0f,1.0f,1.0f };
		float m_brightness = 1.0f;	
		float m_distance = 32.0f;	
		float m_attenuation = 1.0f;	
		float m_ambient = 0.0f;
	};
}