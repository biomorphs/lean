#pragma once
#include "engine/system.h"
#include "core/glm_headers.h"
#include <memory>

namespace Render
{
	class Texture;
	class ShaderProgram;
}

namespace Engine
{
	class DebugGuiSystem;
	class RenderSystem;
}

class ComputeTest : public Engine::System
{
public:
	ComputeTest();
	virtual ~ComputeTest();
	virtual bool PreInit(Engine::SystemManager& manager);
	virtual bool PostInit();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();

private:
	bool RecompileShader();
	std::unique_ptr<Render::Texture> m_texture;
	std::unique_ptr<Render::ShaderProgram> m_shader;
	glm::ivec2 m_dims = { 1280,720 };
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::RenderSystem* m_renderSys = nullptr;
};