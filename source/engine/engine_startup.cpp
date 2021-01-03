#include "engine_startup.h"
#include "system_manager.h"
#include "event_system.h"
#include "job_system.h"
#include "input_system.h"
#include "render_system.h"
#include "script_system.h"
#include "debug_gui_system.h"
#include "platform.h"
#include "core/log.h"
#include <cassert>

namespace Engine
{
	class EngineSystemRegister : public SystemRegister
	{
	public:
		EngineSystemRegister(SystemManager& sys) : m_systemManager(sys) {}
		virtual void Register(const char* systemName, System* theSystem)
		{
			m_systemManager.RegisterSystem(systemName, theSystem);
		}
		SystemManager& m_systemManager;
	};

	// Application entry point
	int Run(std::function<void(SystemRegister&)> systemCreation, int argc, char* args[])
	{
		// Initialise platform stuff
		Platform::InitResult result = Platform::Initialise(argc, args);
		assert(result == Platform::InitResult::InitOK);
		if (result == Platform::InitResult::InitFailed)
		{
			return 1;
		}

		// Create the system manager and register systems
		SystemManager sysManager;
		sysManager.RegisterSystem("Events", new EventSystem);
		sysManager.RegisterSystem("Jobs", new JobSystem);
		sysManager.RegisterSystem("Input", new InputSystem);
		sysManager.RegisterSystem("Scripts", new ScriptSystem);
		sysManager.RegisterSystem("DebugGui", new DebugGuiSystem);

		SDE_LOGC(Engine, "Creating systems...");
		EngineSystemRegister registerSystems(sysManager);
		systemCreation(registerSystems);

		sysManager.RegisterSystem("Render", new RenderSystem);

		// Run the engine
		SDE_LOGC(Engine, "Initialising systems...");
		bool initResult = sysManager.Initialise();
		assert(initResult);
		if (initResult == true)
		{
			SDE_LOGC(Engine, "Running engine main loop");
			while (sysManager.Tick())
			{
			}
		}

		SDE_LOGC(Engine, "Shutting down systems");
		sysManager.Shutdown();

		// Shutdown
		auto shutdownResult = Platform::Shutdown();
		assert(shutdownResult == Platform::ShutdownResult::ShutdownOK);
		return shutdownResult == Platform::ShutdownResult::ShutdownOK ? 0 : 1;
	}
}