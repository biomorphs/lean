#include "engine_startup.h"
#include "system_manager.h"
#include "event_system.h"
#include "job_system.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "model_manager.h"
#include "input_system.h"
#include "render_system.h"
#include "script_system.h"
#include "graphics_system.h"
#include "debug_gui_system.h"
#include "physics_system.h"
#include "sdf_mesh_system.h"
#include "camera_system.h"
#include "raycast_system.h"
#include "character_controller_system.h"
#include "entity/entity_system.h"
#include "platform.h"
#include "core/log.h"
#include "core/timer.h"
#include "core/random.h"
#include <cassert>

namespace Engine
{
	// Application entry point
	int Run(std::function<void()> systemCreation, int argc, char* args[])
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
		auto raycaster = new RaycastSystem;

		SystemManager& sysManager = SystemManager::GetInstance();
		sysManager.RegisterSystem("Events", new EventSystem);
		sysManager.RegisterSystem("Jobs", new JobSystem);
		sysManager.RegisterSystem("Input", new InputSystem);
		sysManager.RegisterSystem("Script", new ScriptSystem);
		sysManager.RegisterSystem("DebugGui", new DebugGuiSystem);
		sysManager.RegisterSystem("PhysicsEntities", physics->MakeUpdater());
		sysManager.RegisterSystem("RaycastResults", raycaster->MakeResultProcessor());
		systemCreation();
		sysManager.RegisterSystem("CharacterControllers", new CharacterControllerSystem());
		sysManager.RegisterSystem("Entities", new EntitySystem);
		sysManager.RegisterSystem("SDFMeshes", new SDFMeshSystem);
		sysManager.RegisterSystem("Graphics", new GraphicsSystem);
		sysManager.RegisterSystem("Raycasts", raycaster);
		sysManager.RegisterSystem("Cameras", new CameraSystem);
		sysManager.RegisterSystem("Textures", new TextureManager);
		sysManager.RegisterSystem("Models", new ModelManager);
		sysManager.RegisterSystem("Shaders", new ShaderManager);
		sysManager.RegisterSystem("Render", render);
		sysManager.RegisterSystem("Physics", physics);
		sysManager.RegisterSystem("RenderPresent", render->MakePresenter());
		sysManager.RegisterSystem("ShaderHotreload", new ShaderManager::HotReloader);

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