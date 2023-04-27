#include "engine/engine_startup.h"
#include "engine/system_manager.h"

#include "playground/playground.h"
#include "playground/walkable_system.h"

#include "particles/emitter_editor.h"

#include "survivors/world_tile_system.h"
#include "survivors/survivors_main.h"

#include "editor/editor.h"

#include "graphs/graph_system.h"

#include "behaviours/behaviour_tree_system.h"

#include "core/log.h"
#include <algorithm>

void CreateSystems(const std::string& cmdLine)
{
	auto& sysManager = Engine::SystemManager::GetInstance();
	sysManager.RegisterSystem("Walkables", new WalkableSystem);
	sysManager.RegisterSystem("SurvivorsWorldTiles", new Survivors::WorldTileSystem);
	sysManager.RegisterSystem("SurvivorsMain", new Survivors::SurvivorsMain);
	//sysManager.RegisterSystem("Graphs", new GraphSystem);
	if (cmdLine.find("-playground") != -1)
	{
		sysManager.RegisterSystem("Playground", new Playground());
	}
	if (cmdLine.find("-editor") != -1)
	{
		sysManager.RegisterSystem("Editor", new Editor());
		sysManager.RegisterSystem("EmitterEditor", new Particles::EmitterEditor());
	}
	sysManager.RegisterSystem("Behaviours", new Behaviours::BehaviourTreeSystem());
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
