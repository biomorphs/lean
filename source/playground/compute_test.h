#pragma once
#include "engine/system.h"
#include "engine/shader_manager.h"
#include "core/glm_headers.h"
#include <memory>

namespace Render
{
	class Texture;
	class ShaderProgram;
	class FrameBuffer;
	class RenderTargetBlitter;
}

namespace Engine
{
	class DebugGuiSystem;
	class RenderSystem;
}
class GraphicsSystem;

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
	Engine::ShaderHandle m_blitShader;
	std::unique_ptr<Render::FrameBuffer> m_debugFrameBuffer;
	std::unique_ptr<Render::Texture> m_volumeTexture;
	std::unique_ptr<Render::ShaderProgram> m_shader;
	std::unique_ptr<Render::RenderTargetBlitter> m_blitter;
	glm::ivec3 m_dims = { 128,128,128 };
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::RenderSystem* m_renderSys = nullptr;
	GraphicsSystem* m_graphics = nullptr;
};