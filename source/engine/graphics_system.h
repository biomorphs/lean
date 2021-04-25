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
	class DebugCamera;
	class ArcballCamera;
	class JobSystem;
	class DebugGuiSystem;
	class TextureManager;
	class ModelManager;
	class ShaderManager;
	class Renderer;
	class DebugRender;
	class Model;	
}
namespace Render
{
	class Camera;
}
class EntitySystem;

class GraphicsSystem : public Engine::System
{
public:
	GraphicsSystem();
	virtual ~GraphicsSystem();
	virtual bool PreInit(Engine::SystemEnumerator& systemEnumerator);
	virtual bool Initialise();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();
	Engine::DebugRender& DebugRenderer() { return *m_debugRender; }
private:
	void ShowGui(int fps);
	void ProcessLight(class Light& l, const class Transform* transform);
	void ProcessEntities();
	void ProcessCamera(float timeDelta);
	void DrawModelBounds(const Engine::Model& m, glm::mat4 transform);
	bool m_showBounds = false;
	std::unique_ptr<Engine::DebugRender> m_debugRender;
	std::unique_ptr<Engine::Renderer> m_renderer;
	std::unique_ptr<Engine::TextureManager> m_textures;
	std::unique_ptr<Engine::ModelManager> m_models;
	std::unique_ptr<Engine::ShaderManager> m_shaders;
	std::unique_ptr<Engine::DebugCamera> m_debugCamera;
	std::unique_ptr<Engine::ArcballCamera> m_arcballCamera;
	std::unique_ptr<Render::Camera> m_mainRenderCamera;
	glm::ivec2 m_windowSize = { 0,0 };
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	Engine::RenderSystem* m_renderSystem = nullptr;
	Engine::JobSystem* m_jobSystem = nullptr;
	Engine::InputSystem* m_inputSystem = nullptr;
	EntitySystem* m_entitySystem = nullptr;
};