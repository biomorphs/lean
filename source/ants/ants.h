#pragma once

#include "engine/system.h"

class AntsSystem : public Engine::System
{
public:
	bool PostInit();
	bool Tick(float timeDelta);
	void DropAllItems(const class EntityHandle& ant);
private:
	void KillAnt(const class EntityHandle& e);
};