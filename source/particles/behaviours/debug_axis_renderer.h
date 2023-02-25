#pragma once 
#include "particles/particle_emission_behaviour.h"

namespace Particles
{
	class DebugAxisRenderer : public RenderBehaviour
	{
	public:
		SERIALISED_CLASS();
		virtual void Draw(double emitterAge, float deltaTime, ParticleContainer& container);
		virtual std::unique_ptr<RenderBehaviour> MakeNew() { return std::make_unique<DebugAxisRenderer>(); }
		virtual std::string_view GetName() { return "Debug Axis Renderer"; }
		virtual void Inspect(EditorValueInspector&);
		float m_axisScale = 1.0f;
	};
}