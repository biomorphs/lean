#include "component_light.h"
#include "render/frame_buffer.h"
#include "engine/debug_gui_system.h"
#include "engine/frustum.h"
#include "engine/debug_render.h"
#include "entity/entity_handle.h"

COMPONENT_SCRIPTS(Light,
	"SetPointLight", &Light::SetPointLight,
	"SetDirectional", &Light::SetDirectional,
	"SetSpotLight", &Light::SetSpotLight,
	"SetColour", &Light::SetColour3,
	"SetBrightness", &Light::SetBrightness,
	"SetDistance", &Light::SetDistance,
	"SetAttenuation", &Light::SetAttenuation,
	"SetAmbient", &Light::SetAmbient,
	"SetCastsShadows", &Light::SetCastsShadows,
	"SetShadowmapSize", &Light::SetShadowmapSize,
	"SetShadowBias", &Light::SetShadowBias,
	"SetSpotAngles", &Light::SetSpotAngles,
	"SetShadowOrthoScale", &Light::SetShadowOrthoScale
)

SERIALISE_BEGIN(Light)
SERIALISE_END()

glm::mat4 Light::UpdateShadowMatrix(glm::vec3 position, glm::vec3 direction)
{
	if (m_type == Type::Point)
	{
		// todo: parameterise
		auto lightPos = glm::vec3(position);
		float aspect = m_shadowMap->Dimensions().x / (float)m_shadowMap->Dimensions().y;
		float near = 0.1f;
		float far = m_distance;
		m_shadowMatrix = glm::perspective(glm::radians(90.0f), aspect, near, far);	// return a 90 degree frustum used to render cubemap
	}
	else if(m_type == Type::Directional)
	{
		// todo - parameterise
		static float c_nearPlane = 0.1f;
		const glm::vec3 up = direction.y == -1.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		glm::mat4 lightProjection = glm::ortho(-m_shadowOrthoScale, m_shadowOrthoScale, -m_shadowOrthoScale, m_shadowOrthoScale, c_nearPlane, m_distance);
		glm::mat4 lightView = glm::lookAt(glm::vec3(position), glm::vec3(position) + direction, up);
		m_shadowMatrix = lightProjection * lightView;
	}
	else if (m_type == Type::Spot)
	{
		static float c_nearPlane = 0.1f;
		const glm::vec3 up = direction.y == -1.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		glm::mat4 lightView = glm::lookAt(glm::vec3(position), glm::vec3(position) + direction, up);
		float aspect = m_shadowMap->Dimensions().x / (float)m_shadowMap->Dimensions().y;
		float near = 0.1f;
		float far = m_distance;
		m_shadowMatrix = glm::perspective(acosf(m_spotAngles.y) * 2.0f, aspect, near, far) * lightView;
	}
	return m_shadowMatrix;
}

void Light::SetType(Type newType)
{
	m_type = newType;
}

void Light::SetSpotLight()
{
	SetType(Type::Spot);
}

void Light::SetDirectional()
{
	SetType(Type::Directional);
}

void Light::SetPointLight()
{
	SetType(Type::Point);
}

void Light::SetCastsShadows(bool c)
{
	m_castShadows = c; 
}

COMPONENT_INSPECTOR_IMPL(Light, Engine::DebugGuiSystem& gui, Engine::DebugRender& render)
{
	auto fn = [&gui, &render](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& l = *static_cast<Light::StorageType&>(cs).Find(e);
		int typeIndex = static_cast<int>(l.GetLightType());
		const char* types[] = { "Directional", "Point", "Spot" };
		if (gui.ComboBox("Type", types, 3, typeIndex))
		{
			l.SetType(static_cast<Light::Type>(typeIndex));
		}
		l.SetColour(glm::vec3(gui.ColourEdit("Colour", glm::vec4(l.GetColour(), 1.0f), false)));
		l.SetBrightness(gui.DragFloat("Brightness", l.GetBrightness(), 0.001f, 0.0f, 10000.0f));
		l.SetAmbient(gui.DragFloat("Ambient", l.GetAmbient(), 0.001f, 0.0f, 1.0f));
		l.SetDistance(gui.DragFloat("Distance", l.GetDistance(), 0.1f, 0.0f, 3250.0f));
		if (l.GetLightType() != Light::Type::Directional)
		{
			l.SetAttenuation(gui.DragFloat("Attenuation", l.GetAttenuation(), 0.1f, 0.0001f, 1000.0f));
		}
		if (l.GetLightType() == Light::Type::Spot)
		{
			auto angles = l.GetSpotAngles();
			angles.y = gui.DragFloat("Outer Angle", angles.y, 0.01f, angles.x, 1.0f);
			angles.x = gui.DragFloat("Inner Angle", angles.x, 0.01f, 0.0f, angles.y);
			l.SetSpotAngles(angles.x, angles.y);
		}
		l.SetCastsShadows(gui.Checkbox("Cast Shadows", l.CastsShadows()));
		if (l.CastsShadows())
		{
			if (l.GetLightType() == Light::Type::Directional)
			{
				l.SetShadowOrthoScale(gui.DragFloat("Ortho Scale", l.GetShadowOrthoScale(), 0.1f, 0.1f, 10000000.0f));
			}
			l.SetShadowBias(gui.DragFloat("Shadow Bias", l.GetShadowBias(), 0.001f, 0.0f, 10.0f));
			if (!l.IsPointLight() && l.GetShadowMap() != nullptr && !l.GetShadowMap()->IsCubemap())
			{
				gui.Image(*l.GetShadowMap()->GetDepthStencil(), glm::vec2(256.0f));
			}
		}
		if (!l.IsPointLight())
		{
			Engine::Frustum f(l.GetShadowMatrix());
			render.DrawFrustum(f, glm::vec4(l.GetColour(), 1.0f));
		}
	};
	return fn;
}