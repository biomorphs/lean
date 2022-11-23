#pragma once

#include "entity/component.h"

class DeadMonsterComponent
{
public:
	COMPONENT(DeadMonsterComponent);
	COMPONENT_INSPECTOR();

	void SetDeathTime(double t) { m_deathTime = t; }
	double GetDeathTime() { return m_deathTime; }

private:
	double m_deathTime = 0;
};