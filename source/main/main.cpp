#include "engine/engine_startup.h"
#include "playground/playground.h"
#include "editor/editor.h"
#include "entity/entity_system.h"
#include "core/log.h"
#include <algorithm>

class CreateSystems
{
public:
	CreateSystems(std::string cmdLine)
		: m_cmdline(cmdLine)
	{
	}
	void operator()(Engine::SystemRegister& r)
	{
		//if (m_cmdline.find("-playground") != -1)
		{
			r.Register("Playground", new Playground());
		}
		//else
		{
			r.Register("Editor", new Editor());
		}
	}

private:
	std::string m_cmdline;
};

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
	return Engine::Run(CreateSystems(fullCmdLine), 0, nullptr);
}
