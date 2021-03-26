#include "light.h"
#include "render/frame_buffer.h"

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