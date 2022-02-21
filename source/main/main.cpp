#include "engine/engine_startup.h"
#include "engine/system_manager.h"

#include "playground/playground.h"
#include "playground/walkable_system.h"

#include "editor/editor.h"

#include "core/log.h"
#include <algorithm>

void CreateSystems(const std::string& cmdLine)
{
	auto& sysManager = Engine::SystemManager::GetInstance();
	sysManager.RegisterSystem("Walkables", new WalkableSystem);
	if (cmdLine.find("-playground") != -1)
	{
		sysManager.RegisterSystem("Playground", new Playground());
	}
	if (cmdLine.find("-editor") != -1)
	{
		sysManager.RegisterSystem("Editor", new Editor());
	}
}

int main(int argc, char** args)
{
	std::string fullCmdLine;
	if (argc > 1)
	{
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
	auto createUserSystems = [fullCmdLine]()
	{
		CreateSystems(fullCmdLine);
	};
	return Engine::Run(createUserSystems, argc, args);
}
