#include "engine_startup.h"
#include "system_manager.h"
#include "event_system.h"
#include "job_system.h"
#include "input_system.h"
#include "render_system.h"
#include "script_system.h"
#include "graphics_system.h"
#include "debug_gui_system.h"
#include "physics_system.h"
#include "sdf_mesh_system.h"
#include "camera_system.h"
#include "entity/entity_system.h"
#include "platform.h"
#include "core/log.h"
#include "core/timer.h"
#include "core/random.h"
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

		// Init random number generator
		Core::Random::ResetGlobalSeed();

		auto physics = new PhysicsSystem;
		auto render = new RenderSystem;

		SystemManager sysManager;
		sysManager.RegisterSystem("Events", new EventSystem);
		sysManager.RegisterSystem("Jobs", new JobSystem);
		sysManager.RegisterSystem("Input", new InputSystem);
		sysManager.RegisterSystem("Script", new ScriptSystem);
		sysManager.RegisterSystem("DebugGui", new DebugGuiSystem);
		sysManager.RegisterSystem("PhysicsEntities", physics->MakeUpdater());
		
		EngineSystemRegister registerSystems(sysManager);
		systemCreation(registerSystems);

		sysManager.RegisterSystem("SDFMeshes", new SDFMeshSystem);
		sysManager.RegisterSystem("Entities", new EntitySystem);
		sysManager.RegisterSystem("Graphics", new GraphicsSystem);
		sysManager.RegisterSystem("Physics", physics);
		sysManager.RegisterSystem("Cameras", new CameraSystem);
		sysManager.RegisterSystem("Render", render);
		sysManager.RegisterSystem("RenderPresent", render->MakePresenter());

		// Run the engine
		SDE_LOGC(Engine, "Initialising systems...");
		bool initResult = sysManager.Initialise();
		assert(initResult);
		if (initResult == true)
		{
			SDE_LOGC(Engine, "Running engine main loop");
			Core::Timer engineTime;
			double startTime = engineTime.GetSeconds();
			double lastTickTime = startTime;
			bool running = true;
			while (running)
			{
				double thisTime = engineTime.GetSeconds();
				double deltaTime = glm::clamp(thisTime - lastTickTime, 0.0047, 0.033);
				lastTickTime = thisTime;
				running = sysManager.Tick(deltaTime);
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