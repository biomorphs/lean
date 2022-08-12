#pragma once
#include "engine/system.h"
#include "core/timer.h"
#include "core/glm_headers.h"
#include <vector>
#include <string>
#include <memory>

namespace Engine
{
	class InputSystem;
	class ScriptSystem;
	class RenderSystem;
	class JobSystem;
	class DebugGuiSystem;
	class ShaderManager;
	class Renderer;
	class NewRender;
	class DebugRender;
	class Model;	
}
class EntitySystem;

class GraphicsSystem : public Engine::System
{
public:
	GraphicsSystem();
	virtual ~GraphicsSystem();
	virtual bool PreInit();
	virtual bool PostInit();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();
	Engine::DebugRender& DebugRenderer() { return *m_debugRender; }
	Engine::Renderer& Renderer() { return *m_renderer; }
	Engine::NewRender& NewRender() { return *m_newRender; }
	bool ShouldDrawBounds() const { return m_showBounds; }
	void DrawModelBounds(const Engine::Model& m, glm::mat4 transform, glm::vec4 mainColour, glm::vec4 partsColour = glm::vec4(0.0f));
private:
	void RegisterScripts();
	void RegisterComponents();
	void ShowGui(int fps);
	void ShowRTGui();
	void ProcessLight(class Light& l, class Transform* transform);
	void ProcessEntities();
	bool m_showBounds = false;
	bool m_showStats = false;
	bool m_showTargets = false;
	bool m_useNewRender = false;
	std::unique_ptr<Engine::DebugRender> m_debugRender;
	std::unique_ptr<Engine::Renderer> m_renderer;
	std::unique_ptr<Engine::NewRender> m_newRender;
	glm::ivec2 m_windowSize = { 0,0 };
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	Engine::RenderSystem* m_renderSystem = nullptr;
	Engine::JobSystem* m_jobSystem = nullptr;
	Engine::InputSystem* m_inputSystem = nullptr;
	EntitySystem* m_entitySystem = nullptr;
};