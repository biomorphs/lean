#pragma once
#include "engine/system.h"

class WalkableSystem : public Engine::System
{
public:
	WalkableSystem();
	virtual ~WalkableSystem();
	virtual bool PreInit();
	virtual bool Initialise();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();
private:
	void RegisterScripts();
	void RegisterComponents();
};