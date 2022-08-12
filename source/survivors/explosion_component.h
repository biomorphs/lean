#pragma once

#include "entity/component.h"

// causes damage in an area once
class ExplosionComponent
{
public:
	COMPONENT(ExplosionComponent);
	bool HasExploded() { return m_hasExploded; }
	void SetHasExploded(bool e) { m_hasExploded = e; }
	float GetDamageRadius() { return m_damageRadius; }
	void SetDamageRadius(float v) { m_damageRadius = v; }
	float GetDamageAtCenter() { return m_damageAtCenter; }
	void SetDamageAtCenter(float v) { m_damageAtCenter = v; }
	float GetDamageAtEdge() { return m_damageAtEdge; }
	void SetDamageAtEdge(float v) { m_damageAtEdge = v; }
	float GetFadeoutSpeed() { return m_fadeOutSpeed; }
	void SetFadeoutSpeed(float v) { m_fadeOutSpeed = v; }
private:
	bool m_hasExploded = false;
	float m_damageRadius = 64.0f;
	float m_damageAtCenter = 100.0f;
	float m_damageAtEdge = 10.0f;
	float m_fadeOutSpeed = 4.0f;
};