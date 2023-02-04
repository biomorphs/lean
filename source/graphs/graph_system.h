#include "engine/system.h"

class GraphSystem : public Engine::System
{
public:
	virtual ~GraphSystem() {}
	bool Initialise();
	bool Tick(float timeDelta);
	void Shutdown();
};