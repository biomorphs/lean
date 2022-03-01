#include "component_light.h"
#include "render/frame_buffer.h"
#include "engine/debug_gui_system.h"
#include "engine/frustum.h"
#include "engine/debug_render.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"

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
SERIALISE_PROPERTY("Type", m_type)
SERIALISE_PROPERTY("Colour", m_colour)
SERIALISE_PROPERTY("Brightness", m_brightness)
SERIALISE_PROPERTY("Distance", m_distance)
SERIALISE_PROPERTY("Attenutation", m_attenuation)
SERIALISE_PROPERTY("SpotAngles", m_spotAngles)
SERIALISE_PROPERTY("Ambient", m_ambient)
SERIALISE_PROPERTY("ShadowBias", m_shadowBias)
SERIALISE_PROPERTY("ShadowOrthoScale", m_shadowOrthoScale)
SERIALISE_PROPERTY("CastShadows", m_castShadows)
SERIALISE_PROPERTY("ShadowMapSize", m_shadowMapSize)
SERIALISE_END()

glm::mat4 Light::UpdateShadowMatrix(glm::vec3 position, glm::vec3 direction)
{
	const float c_nearPlane = 0.1f;	// parameterise?
	if (m_type == Type::Point)
	{
		const auto lightPos = glm::vec3(position);
		const float aspect = m_shadowMap->Dimensions().x / (float)m_shadowMap->Dimensions().y;
		const float far = m_distance;
		m_shadowMatrix = glm::perspective(glm::radians(90.0f), aspect, c_nearPlane, far);	// return a 90 degree frustum used to render cubemap
	}
	else if(m_type == Type::Directional)
	{
		const glm::vec3 up = direction.y == -1.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::mat4 lightProjection = glm::ortho(-m_shadowOrthoScale, m_shadowOrthoScale, -m_shadowOrthoScale, m_shadowOrthoScale, c_nearPlane, m_distance);
		const glm::mat4 lightView = glm::lookAt(glm::vec3(position), glm::vec3(position) + direction, up);
		m_shadowMatrix = lightProjection * lightView;
	}
	else if (m_type == Type::Spot)
	{
		const glm::vec3 up = direction.y == -1.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::mat4 lightView = glm::lookAt(glm::vec3(position), glm::vec3(position) + direction, up);
		const float aspect = m_shadowMap->Dimensions().x / (float)m_shadowMap->Dimensions().y;
		const float far = m_distance;
		m_shadowMatrix = glm::perspective(acosf(m_spotAngles.y) * 2.0f, aspect, c_nearPlane, far) * lightView;
	}
	return m_shadowMatrix;
}

void Light::SetType(int newType)
{
	m_type = static_cast<Light::Type>(newType);
}

void Light::SetSpotLight()
{
	SetType(static_cast<int>(Type::Spot));
}

void Light::SetDirectional()
{
	SetType(static_cast<int>(Type::Directional));
}

void Light::SetPointLight()
{
	SetType(static_cast<int>(Type::Point));
}

void Light::SetCastsShadows(bool c)
{
	m_castShadows = c; 
}

COMPONENT_INSPECTOR_IMPL(Light, Engine::DebugGuiSystem& gui, Engine::DebugRender& render)
{
	auto fn = [&gui, &render](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& l = *static_cast<Light::StorageType&>(cs).Find(e);
		
		const char* types[] = { "Directional", "Point", "Spot" };
		i.InspectEnum("Type", static_cast<int>(l.GetLightType()), InspectFn(e, &Light::SetType), types, 3);

		i.InspectColour("Colour", l.GetColour(), InspectFn(e, &Light::SetColour));
		i.Inspect("Brightness", l.GetBrightness(), InspectFn(e, &Light::SetBrightness), 0.001f, 0.0f, 10000.0f);
		i.Inspect("Ambient", l.GetAmbient(), InspectFn(e, &Light::SetAmbient), 0.001f, 0.0f, 10000.0f);
		i.Inspect("Distance", l.GetDistance(), InspectFn(e, &Light::SetDistance), 0.1f, 0.0f, 10000.0f);
		if (l.GetLightType() != Light::Type::Directional)
		{
			i.Inspect("Attenuation", l.GetAttenuation(), InspectFn(e, &Light::SetAttenuation), 0.1f, 0.0001f, 1000.0f);
		}
		if (l.GetLightType() == Light::Type::Spot)
		{
			i.Inspect("Outer Angle", l.GetSpotAngles().y, InspectFn (e, &Light::SetSpotOuterAngle), 0.01f, l.GetSpotAngles().x, 1.0f);
			i.Inspect("Inner Angle", l.GetSpotAngles().x, InspectFn(e, &Light::SetSpotInnerAngle), 0.01f, 0.0f, l.GetSpotAngles().y);
		}
		i.Inspect("Cast Shadows", l.CastsShadows(), InspectFn(e, &Light::SetCastsShadows));
		if (l.CastsShadows())
		{
			if (l.GetLightType() == Light::Type::Directional)
			{
				i.Inspect("Ortho Scale", l.GetShadowOrthoScale(), InspectFn(e, &Light::SetShadowOrthoScale), 0.1f, 0.1f, 10000000.0f);
			}
			i.Inspect("Shadow Bias", l.GetShadowBias(), InspectFn(e, &Light::SetShadowBias), 0.001f, 0.0f, 10.0f);
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