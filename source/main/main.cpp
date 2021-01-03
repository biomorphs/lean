
//#include "debug_gui/debug_gui_system.h"
#include "engine/engine_startup.h"
//#include "core/system_registrar.h"
//#include "playground.h"
//#include "graphics.h"

#include "platform.h"

void CreateSystems(Engine::SystemRegister& r)
{
	//systemManager.RegisterSystem("DebugGui", new DebugGui::DebugGuiSystem());
	//systemManager.RegisterSystem("Playground", new Playground());
	//systemManager.RegisterSystem("Graphics", new Graphics());
}

int main()
{
	return Engine::Run(CreateSystems, 0, nullptr);
}
