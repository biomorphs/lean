#include "camera_system.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"
#include "engine/render_system.h"
#include "engine/graphics_system.h"
#include "engine/input_system.h"
#include "engine/debug_camera.h"
#include "engine/renderer.h"
#include "render/window.h"
#include "render/camera.h"
#include "core/profiler.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_camera.h"

namespace Engine
{
	CameraSystem::CameraSystem()
	{
	}

	CameraSystem::~CameraSystem()
	{
	}

	void CameraSystem::RegisterComponents()
	{
		m_entitySystem->RegisterComponentType<Camera>();
		m_entitySystem->RegisterInspector<Camera>(Camera::MakeInspector());
	}

	void CameraSystem::RegisterScripts()
	{
		//// expose script functions
		auto graphics = m_scriptSystem->Globals()["Graphics"].get_or_create<sol::table>();
		graphics["SetActiveCamera"] = [this](EntityHandle c) {
			m_activeCamera = c;
		};
	}

	bool CameraSystem::PreInit()
	{
		SDE_PROF_EVENT();

		m_scriptSystem = Engine::GetSystem<Engine::ScriptSystem>("Script");
		m_renderSystem = Engine::GetSystem<Engine::RenderSystem>("Render");
		m_inputSystem = Engine::GetSystem<Engine::InputSystem>("Input");
		m_debugGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		m_entitySystem = Engine::GetSystem<EntitySystem>("Entities");
		m_graphics = Engine::GetSystem<GraphicsSystem>("Graphics");

		m_mainRenderCamera = std::make_unique<Render::Camera>();

		return true;
	}

	bool CameraSystem::Initialise()
	{
		SDE_PROF_EVENT();

		const auto& windowProps = m_renderSystem->GetWindow()->GetProperties();
		m_windowSize = glm::ivec2(windowProps.m_sizeX, windowProps.m_sizeY);

		m_debugCamera = std::make_unique<Engine::DebugCamera>();
		m_debugCamera->SetPosition({ 60.65f,101.791f,82.469f });
		m_debugCamera->SetPitch(-0.438);
		m_debugCamera->SetYaw(0.524f);

		RegisterComponents();
		RegisterScripts();

		return true;
	}

	bool CameraSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		// Update the camera menu
		Engine::MenuBar mainMenu;
		auto& cameraMenu = mainMenu.AddSubmenu(ICON_FK_CAMERA " Camera");
		cameraMenu.AddItem("Flycam", [this]() {
			m_activeCamera = -1;
		});
		auto& entityMenu = cameraMenu.AddSubmenu("Entities");
		m_entitySystem->GetWorld()->ForEachComponent<Camera>([this, &entityMenu](Camera& c, EntityHandle e) {
			entityMenu.AddItem(m_entitySystem->GetEntityNameWithTags(e), [e, this]() {
				m_activeCamera = e;
				});
			});
		m_debugGui->MainMenuBar(mainMenu);

		auto aspectRatio = (float)m_windowSize.x / (float)m_windowSize.y;
		if (!m_activeCamera.IsValid())
		{
			m_debugCamera->Update(m_inputSystem->ControllerState(0), timeDelta);
			if (!m_debugGui->IsCapturingMouse() && !m_debugGui->IsCapturingKeyboard())
			{
				if (m_inputSystem->GetKeyboardState().m_keyPressed[Engine::KEY_z])
				{
					m_debugCamera->Update(m_inputSystem->GetMouseState(), timeDelta);
					m_debugCamera->Update(m_inputSystem->GetKeyboardState(), timeDelta);
				}
			}
			m_debugCamera->ApplyToCamera(*m_mainRenderCamera);
			m_mainRenderCamera->SetProjection(70.0f, aspectRatio, 0.1f, 8000.0f);
		}
		else
		{
			auto camComp = m_entitySystem->GetWorld()->GetComponent<Camera>(m_activeCamera);
			auto transform = m_entitySystem->GetWorld()->GetComponent<Transform>(m_activeCamera);
			if (camComp && transform)
			{
				// apply component values to main render camera
				m_mainRenderCamera->SetFOVAndAspectRatio(camComp->GetFOV(), aspectRatio);
				m_mainRenderCamera->SetClipPlanes(camComp->GetNearPlane(), camComp->GetFarPlane());
				glm::vec3 lookDirection = glm::vec3(0.0f, 0.0f, 1.0f) * transform->GetOrientation();
				glm::vec3 lookUp = glm::vec3(0.0f, 1.0f, 0.0f) * transform->GetOrientation();
				m_mainRenderCamera->LookAt(transform->GetPosition(), transform->GetPosition() + lookDirection, lookUp);
			}
		}
		m_graphics->Renderer().SetCamera(*m_mainRenderCamera);

		return true;
	}

	void CameraSystem::Shutdown()
	{
		SDE_PROF_EVENT();
	}
}