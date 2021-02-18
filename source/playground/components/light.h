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

	enum class Type
	{
		Directional = 0,
		Point,
		Spot,
	};
	
	void SetType(Type newType);
	void SetDirectional();
	void SetPointLight();
	void SetColour(float r, float g, float b) { m_colour = { r,g,b }; }
	void SetBrightness(float b) { m_brightness = b; }
	void SetDistance(float d) { m_distance = d; }
	void SetSpotAnglesDegrees(float inner, float outer) { m_spotAngles = glm::radians(glm::vec2(inner, outer)); }
	void SetAmbient(float a) { m_ambient = a; }
	void SetCastsShadows(bool c);
	void SetShadowmapSize(int w, int h) { m_shadowMapSize = { w,h }; }
	void SetShadowBias(float b) { m_shadowBias = b; }
	glm::mat4 UpdateShadowMatrix(glm::vec3 position, glm::vec3 direction);

	Type GetLightType() const { return m_type; }
	std::unique_ptr<Render::FrameBuffer>& GetShadowMap() { return m_shadowMap; }
	glm::ivec2 GetShadowmapSize() const { return m_shadowMapSize; }
	bool CastsShadows() const { return m_castShadows; }
	bool IsPointLight() const { return m_type == Type::Point; }
	glm::vec3 GetColour() const { return m_colour; }
	float GetBrightness() const { return m_brightness; }
	float GetAmbient() const { return m_ambient; }
	float GetDistance() const { return m_distance; }
	float GetShadowBias() const { return m_shadowBias; }
	glm::vec3 GetAttenuation() const;
	glm::vec2 GetSpotAngles() const { return m_spotAngles; }	// radians!
	glm::mat4 GetShadowMatrix() { return m_shadowMatrix; }

private:
	Type m_type = Type::Directional;
	glm::vec3 m_colour = { 1.0f,1.0f,1.0f };
	float m_brightness = 1.0f;	// scales colour (HDR)
	float m_distance = 32.0f;	// used to calculate attenuation
	glm::vec2 m_spotAngles = { 0.0f,0.0f };	// radians
	float m_ambient = 0.0f;
	float m_shadowBias = 0.05f;	// functionality may depend on light type
	bool m_castShadows = false;
	glm::ivec2 m_shadowMapSize = { 256,256 };
	glm::mat4 m_shadowMatrix = glm::identity<glm::mat4>();	// lightspace matrix
	std::unique_ptr<Render::FrameBuffer> m_shadowMap;	// can be 2d or 3d (cubemap)
};