#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"

class Light : public Component
{
public:
	COMPONENT(Light);
	
	void SetIsPointLight(bool b) { m_isPointLight = b; }
	void SetColour(float r, float g, float b) { m_colour = { r,g,b }; }
	void SetDistance(float d) { m_distance = d; }
	void SetAmbient(float a) { m_ambient = a; }
	bool IsPointLight() const { return m_isPointLight; }
	glm::vec3 GetColour() const { return m_colour; }
	float GetAmbient() const { return m_ambient; }
	glm::vec3 GetAttenuation() const;

private:
	glm::vec3 m_colour = { 1.0f,1.0f,1.0f };
	float m_distance = 32.0f;	// used to calculate attenuation
	float m_ambient = 0.0f;
	bool m_isPointLight = true;
};