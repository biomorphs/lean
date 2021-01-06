#include "engine/engine_startup.h"
#include "playground/playground.h"
#include "playground/graphics.h"
#include "engine/entity/entity_system.h"

void CreateSystems(Engine::SystemRegister& r)
{
	r.Register("Playground", new Playground());
	r.Register("Graphics", new Graphics());
}

int main()
{
	return Engine::Run(CreateSystems, 0, nullptr);
}
