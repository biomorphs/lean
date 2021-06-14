#pragma once
#include "engine/system.h"
#include "entity/entity_handle.h"
#include "core/timer.h"
#include "core/glm_headers.h"
#include <vector>
#include <string>
#include <memory>

namespace Render
{
	class Camera;
}
class EntitySystem;
class GraphicsSystem;

namespace Engine
{
	class InputSystem;
	class ScriptSystem;
	class RenderSystem;
	class DebugCamera;
	class DebugGuiSystem;
	class Renderer;

	class CameraSystem : public Engine::System
	{
	public:
		CameraSystem();
		virtual ~CameraSystem();
		virtual bool PreInit(Engine::SystemManager& manager);
		virtual bool Initialise();
		virtual bool Tick(float timeDelta);
		virtual void Shutdown();
		const Render::Camera& MainCamera() const { return *m_mainRenderCamera; }
	private:
		void RegisterScripts();
		void RegisterComponents();
		EntityHandle m_activeCamera;
		std::unique_ptr<Engine::DebugCamera> m_debugCamera;
		std::unique_ptr<Render::Camera> m_mainRenderCamera;
		glm::ivec2 m_windowSize = { 0,0 };
		Engine::DebugGuiSystem* m_debugGui = nullptr;
		Engine::ScriptSystem* m_scriptSystem = nullptr;
		Engine::RenderSystem* m_renderSystem = nullptr;
		Engine::InputSystem* m_inputSystem = nullptr;
		GraphicsSystem* m_graphics = nullptr;
		EntitySystem* m_entitySystem = nullptr;
	};
}