#include "engine/engine_startup.h"
#include "playground/playground.h"
#include "playground/creatures/creature_system.h"
#include "playground/compute_test.h"
#include "entity/entity_system.h"
#include "core/log.h"

void CreateSystems(Engine::SystemRegister& r)
{
	r.Register("Playground", new Playground());
	r.Register("Creatures", new CreatureSystem());
	r.Register("ComputeTest", new ComputeTest());
}

int main(int argc, char** args)
{
	if (argc > 1)
	{
		std::string fullCmdLine = "";
		for (int i = 1; i < argc; ++i)
		{
			fullCmdLine += args[i];
		}
		if (fullCmdLine.find("-waitforprofiler") != std::string::npos)
		{
			SDE_LOG("Waiting for profiler connection...");
			while (!SDE_PROF_IS_ACTIVE())
			{
				SDE_PROF_FRAME("Main Thread");	// kick off the profiler with a fake frame
				SDE_PROF_EVENT();
				_sleep(20);
			}
		}
	}
	return Engine::Run(CreateSystems, 0, nullptr);
}
