#include "attached_light_behaviour.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/file_picker_dialog.h"
#include "engine/renderer.h"
#include "particles/particle_container.h"
#include "particles/editor_value_inspector.h"

namespace Particles
{
	SERIALISE_BEGIN_WITH_PARENT(AttachedLightBehaviour, RenderBehaviour)
	{
		SERIALISE_PROPERTY("AttachToEmitter", m_attachToEmitter);
		SERIALISE_PROPERTY("AttachToParticles", m_attachToParticles);
		SERIALISE_PROPERTY("Colour", m_colour);
		SERIALISE_PROPERTY("Brightness", m_brightness);
		SERIALISE_PROPERTY("Distance", m_distance);
		SERIALISE_PROPERTY("Attenuation", m_attenuation);
		SERIALISE_PROPERTY("Ambient", m_ambient);
	}
	SERIALISE_END()

	void AttachedLightBehaviour::Draw(glm::vec3 emitterPos, glm::quat orientation, double emitterAge, float deltaTime, ParticleContainer& container)
	{
		SDE_PROF_EVENT();
		static auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
		glm::vec3 lightColour = m_colour * m_brightness;
		if (m_attachToEmitter)
		{
			graphics->Renderer().PointLight(emitterPos, lightColour, m_ambient, m_distance, m_attenuation);
		}
		if (m_attachToParticles)
		{
			const uint32_t count = container.AliveParticles();
			__declspec(align(16)) glm::vec4 position;
			for (uint32_t p = 0; p < count; ++p)
			{
				_mm_store_ps(glm::value_ptr(position), container.Positions().GetValue(p));
				graphics->Renderer().PointLight(glm::vec3(position), lightColour, m_ambient, m_distance, m_attenuation);
			}
		}
	}

	void AttachedLightBehaviour::Inspect(EditorValueInspector& v)
	{
		v.Inspect("Attach to emitter", m_attachToEmitter, [this](bool val) {
			m_attachToEmitter = val;
		});
		v.Inspect("Attach to particles", m_attachToParticles, [this](bool val) {
			m_attachToParticles = val;
		});
		v.InspectColour("Colour", m_colour, [this](glm::vec3 val) {
			m_colour = val;
		});
		v.Inspect("Brightness", m_brightness, [this](float val) {
			m_brightness = val;
		});
		v.Inspect("Distance", m_distance, [this](float val) {
			m_distance = val;
		});
		v.Inspect("Attenuation", m_attenuation, [this](float val) {
			m_attenuation = val;
		});
		v.Inspect("Ambient", m_ambient, [this](float val) {
			m_ambient = val;
		});
	}
}
