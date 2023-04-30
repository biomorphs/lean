#pragma once

#include "engine/system.h"

class AntsSystem : public Engine::System
{
public:
	bool PostInit();
	bool Tick(float timeDelta);
private:
	void KillAnt(const class EntityHandle& e);
};