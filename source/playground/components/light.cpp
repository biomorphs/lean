#include "light.h"
#include "render/frame_buffer.h"

COMPONENT_BEGIN(Light,
	"SetIsPointLight", &Light::SetIsPointLight,
	"SetColour", &Light::SetColour,
	"SetDistance", &Light::SetDistance,
	"SetAmbient", &Light::SetAmbient,
	"SetCastsShadows", &Light::SetCastsShadows,
	"SetShadowmapSize", &Light::SetShadowmapSize
)
COMPONENT_END()

void Light::SetIsPointLight(bool b)
{
	if (m_isPointLight != b)
	{
		m_shadowMap = nullptr;	// safe?!
		m_isPointLight = b;
	}
}

void Light::SetCastsShadows(bool c)
{
	m_castShadows = c; 
	if (!m_castShadows)
	{
		m_shadowMap = nullptr;	// safe?! 
	}
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