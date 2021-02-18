#include "light.h"
#include "render/frame_buffer.h"

COMPONENT_BEGIN(Light,
	"SetPointLight", &Light::SetPointLight,
	"SetDirectional", &Light::SetDirectional,
	"SetColour", &Light::SetColour,
	"SetBrightness", &Light::SetBrightness,
	"SetDistance", &Light::SetDistance,
	"SetAmbient", &Light::SetAmbient,
	"SetCastsShadows", &Light::SetCastsShadows,
	"SetShadowmapSize", &Light::SetShadowmapSize,
	"SetShadowBias", &Light::SetShadowBias
)
COMPONENT_END()

glm::mat4 Light::UpdateShadowMatrix(glm::vec3 position, glm::vec3 direction)
{
	if (m_type == Type::Point)
	{
		// todo: parameterise
		auto lightPos = glm::vec3(position);
		float aspect = m_shadowMap->Dimensions().x / (float)m_shadowMap->Dimensions().y;
		float near = 0.1f;
		float far = m_distance * 4.0f;	// ??? todo, attenuation
		m_shadowMatrix = glm::perspective(glm::radians(90.0f), aspect, near, far);	// return a 90 degree frustum used to render cubemap
	}
	else
	{
		// todo - parameterise
		static float c_nearPlane = 0.1f;
		static float c_farPlane = 1000.0f;
		static float c_orthoDims = 400.0f;
		const glm::vec3 up = direction.y == -1.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		glm::mat4 lightProjection = glm::ortho(-c_orthoDims, c_orthoDims, -c_orthoDims, c_orthoDims, c_nearPlane, c_farPlane);
		glm::mat4 lightView = glm::lookAt(glm::vec3(position), glm::vec3(position) + direction, up);
		m_shadowMatrix = lightProjection * lightView;
	}
	return m_shadowMatrix;
}

void Light::SetType(Type newType)
{
	m_type = newType;
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

glm::vec3 Light::GetAttenuation() const
{
	const glm::vec4 c_lightAttenuationTable[] = {
		{7, 1.0, 0.7,1.8},
		{13,1.0,0.35 ,0.44},
		{20,1.0,0.22 ,0.20},
		{32,1.0,0.14,0.07},
		{50,1.0,0.09,0.032},
		{65,1.0,0.07,0.017},
		{100,1.0,0.045,0.0075},
		{160,1.0,0.027,0.0028},
		{200,1.0,0.022,0.0019},
		{325,1.0,0.014,0.0007},
		{600,1.0,0.007,0.0002},
		{3250,1.0,0.0014,0.000007}
	};

	float closestDistance = FLT_MAX;
	glm::vec4 closestValue = c_lightAttenuationTable[0];

	for (int i = 0; i < std::size(c_lightAttenuationTable); ++i)
	{
		float distance = fabsf(m_distance - c_lightAttenuationTable[i].x);
		if (distance < closestDistance)
		{
			closestValue = c_lightAttenuationTable[i];
			closestDistance = distance;
		}
	}
	return { closestValue.y, closestValue.z, closestValue.w };
}