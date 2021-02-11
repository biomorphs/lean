#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"

namespace Render
{
	class FrameBuffer;
}

class Light : public Component
{
public:
	COMPONENT(Light);

	/*enum Type
	{
		Directional,
		Spot,
		Point
	};*/
	
	void SetIsPointLight(bool b);
	void SetColour(float r, float g, float b) { m_colour = { r,g,b }; }
	void SetBrightness(float b) { m_brightness = b; }
	void SetDistance(float d) { m_distance = d; }
	void SetSpotAngleDegrees(float a) { m_spotAngle = glm::radians(a); }
	void SetAmbient(float a) { m_ambient = a; }
	void SetCastsShadows(bool c);
	void SetShadowmapSize(int w, int h) { m_shadowMapSize = { w,h }; }
	void SetShadowBias(float b) { m_shadowBias = b; }
	
	std::unique_ptr<Render::FrameBuffer>& GetShadowMap() { return m_shadowMap; }
	glm::ivec2 GetShadowmapSize() const { return m_shadowMapSize; }
	bool CastsShadows() const { return m_castShadows; }
	bool IsPointLight() const { return m_isPointLight; }
	float GetSpotAngleDegrees() const { return glm::degrees(m_spotAngle); }
	glm::vec3 GetColour() const { return m_colour; }
	float GetBrightness() const { return m_brightness; }
	float GetAmbient() const { return m_ambient; }
	float GetDistance() const { return m_distance; }
	float GetShadowBias() const { return m_shadowBias; }
	glm::vec3 GetAttenuation() const;

private:
	glm::vec3 m_colour = { 1.0f,1.0f,1.0f };
	float m_brightness = 1.0f;	// scales colour (HDR)
	float m_distance = 32.0f;	// used to calculate attenuation
	float m_spotAngle = 0.0f;	// radians
	float m_ambient = 0.0f;
	float m_shadowBias = 0.05f;	// functionality may depend on light type
	bool m_isPointLight = true;
	bool m_castShadows = false;
	glm::ivec2 m_shadowMapSize = { 256,256 };
	std::unique_ptr<Render::FrameBuffer> m_shadowMap;	// can be 2d or 3d (cubemap)
};