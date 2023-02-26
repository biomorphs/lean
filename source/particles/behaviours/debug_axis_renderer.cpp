#include "debug_axis_renderer.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/system_manager.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(DebugAxisRenderer, RenderBehaviour)
	{
		SERIALISE_PROPERTY("AxisScale", m_axisScale);
	}
	SERIALISE_END()

	void DebugAxisRenderer::Draw(double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		const uint32_t count = container.AliveParticles();
		__declspec(align(16)) glm::vec4 position;
		for (uint32_t p = 0; p < count; ++p)
		{
			_mm_store_ps(glm::value_ptr(position), container.Positions().GetValue(p));
			graphics->DebugRenderer().AddAxisAtPoint(position, m_axisScale);
		}
	}

	void DebugAxisRenderer::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Axis Scale", m_axisScale, [this](float v) {
			m_axisScale = v;
		});
	}
}