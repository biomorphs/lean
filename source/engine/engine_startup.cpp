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
#include "time_system.h"
#include "text_system.h"
#include "entity/entity_system.h"
#include "particles/particle_system.h"
#include "platform.h"
#include "core/log.h"
#include "core/timer.h"
#include "core/random.h"
#include "core/file_io.h"
#include "frame_graph.h"
#include <cassert>

namespace Engine
{
	std::unique_ptr<Engine::FrameGraph> MakeDefaultFrameGraph()
	{
		auto fg = std::make_unique<Engine::FrameGraph>();
		auto& root = fg->m_root;
		auto& mainThread = fg->m_root.AddSequence("Main");

		auto& frameStart = mainThread.AddSequence("FrameStart");
		{
			frameStart.AddFn("Time::Tick");
			frameStart.AddFn("Events::Tick");
			frameStart.AddFn("Jobs::Tick");
			frameStart.AddFn("Input::Tick");
			frameStart.AddFn("DebugGui::Tick");
		}

		auto& update = mainThread.AddSequence("Update");
		auto& fixedUpdate = update.AddFixedUpdateSequence("FixedUpdate");
		{
			fixedUpdate.AddFn("Physics::Tick");
			fixedUpdate.AddFn("Raycasts::Tick");
			fixedUpdate.AddFn("PhysicsEntities::Tick");
			fixedUpdate.AddFn("RaycastResults::Tick");
			fixedUpdate.AddSequence("UserFixedUpdate");
		}
		auto& variableUpdate = update.AddSequence("VariableUpdate");
		{
			variableUpdate.AddSequence("UserVariableUpdate");
			variableUpdate.AddFn("Script::Tick");
			variableUpdate.AddFn("Entities::Tick");
			variableUpdate.AddFn("Cameras::Tick");
		}

		auto& render = mainThread.AddSequence("RenderSubmit");
		{
			render.AddFn("Text::Tick");
			render.AddFn("Particles::Tick");
			render.AddFn("SDFMeshes::Tick");
			render.AddFn("Graphics::Tick");
			render.AddFn("Textures::Tick");
			render.AddFn("Models::Tick");
			render.AddFn("Shaders::Tick");
			render.AddFn("PhysicsGui::Tick");
			render.AddFn("Time::UpdateGui");
			render.AddFn("Render::Tick");
			render.AddFn("RenderPresent::Tick");
			render.AddFn("ShaderHotreload::Tick");
		}

		return std::move(fg);
	}

	bool RunUpdateGraph(Engine::FrameGraph& graph)
	{
		SDE_PROF_FRAME("Main Thread");
		SDE_PROF_EVENT();
		Engine::FrameGraph::Node* current = &graph.m_root;
		return current->Run();
	}

	// Application entry point
	int Run(std::function<void()> systemCreation, std::function<void(FrameGraph&)> frameGraphBuildCb, int argc, char* args[])
	{
		// Initialise platform stuff
		Platform::InitResult result = Platform::Initialise(argc, args);
		assert(result == Platform::InitResult::InitOK);
		if (result == Platform::InitResult::InitFailed)
		{
			return 1;
		}

		// Init file system
		Core::InitialisePaths();

		// Init random number generator
		Core::Random::ResetGlobalSeed();

		auto physics = new PhysicsSystem;
		auto render = new RenderSystem;
		auto raycaster = new RaycastSystem;

		SystemManager& sysManager = SystemManager::GetInstance();
		sysManager.RegisterSystem("Time", new TimeSystem);
		sysManager.RegisterSystem("Events", new EventSystem);
		sysManager.RegisterSystem("Jobs", new JobSystem);
		sysManager.RegisterSystem("Input", new InputSystem);
		sysManager.RegisterSystem("DebugGui", new DebugGuiSystem);
		sysManager.RegisterSystem("Script", new ScriptSystem);
		sysManager.RegisterSystem("Text", new TextSystem);
		sysManager.RegisterSystem("Particles", new Particles::ParticleSystem());
		sysManager.RegisterSystem("PhysicsEntities", physics->MakeUpdater());
		sysManager.RegisterSystem("RaycastResults", raycaster->MakeResultProcessor());
		sysManager.RegisterSystem("Entities", new EntitySystem);
		sysManager.RegisterSystem("SDFMeshes", new SDFMeshSystem);
		sysManager.RegisterSystem("Graphics", new GraphicsSystem);
		sysManager.RegisterSystem("Raycasts", raycaster);
		sysManager.RegisterSystem("Cameras", new CameraSystem);
		sysManager.RegisterSystem("Textures", new TextureManager);
		sysManager.RegisterSystem("Models", new ModelManager);
		sysManager.RegisterSystem("Shaders", new ShaderManager);
		sysManager.RegisterSystem("PhysicsGui", physics->MakeGuiTick());
		sysManager.RegisterSystem("Render", render);
		sysManager.RegisterSystem("Physics", physics);
		sysManager.RegisterSystem("RenderPresent", render->MakePresenter());
		sysManager.RegisterSystem("ShaderHotreload", new ShaderManager::HotReloader);
		systemCreation();

		auto frameGraph = MakeDefaultFrameGraph();
		frameGraphBuildCb(*frameGraph);

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
				running = RunUpdateGraph(*frameGraph);
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