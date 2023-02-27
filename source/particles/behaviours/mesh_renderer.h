#pragma once 
#include "particles/particle_emission_behaviour.h"
#include "engine/shader_manager.h"
#include "engine/model_manager.h"

namespace Particles
{
	class MeshRenderer : public RenderBehaviour
	{
	public:
		SERIALISED_CLASS();
		virtual void Draw(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container);
		virtual std::unique_ptr<RenderBehaviour> MakeNew() { return std::make_unique<MeshRenderer>(); }
		virtual std::string_view GetName() { return "Mesh Renderer"; }
		virtual void Inspect(EditorValueInspector&);
		
		Engine::ShaderHandle m_shader;
		Engine::ModelHandle m_model;
	};
}