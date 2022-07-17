#pragma once

#include "entity/component.h"

class PlayerComponent
{
public:
	COMPONENT(PlayerComponent);
	COMPONENT_INSPECTOR();

	int GetCurrentHealth() { return m_currentHP; }

private:
	// active state
	int m_currentHP = 100;
};