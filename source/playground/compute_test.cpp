#include "compute_test.h"
#include "core/log.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/render_system.h"
#include "render/texture.h"
#include "render/texture_source.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"
#include "render/device.h"

// TODO

// MAKE 3D TEXTURE
// WRITE TO IT VIA COMPUTE SHADER
// Show the result (SLICE VIEWER/RAYMARCHER)
// MAKE VB + VArray
// OUTPUT QUADS TO VB AS TEST (AS TRIANGLES!!!)
//	use atomic counter to output triangles
// draw quads using VB with slice shader
// ...
// implement sdf meshing

ComputeTest::ComputeTest()
{
}

ComputeTest::~ComputeTest()
{
}

bool ComputeTest::PreInit(Engine::SystemManager & manager)
{
	m_debugGui = (Engine::DebugGuiSystem*)manager.GetSystem("DebugGui");
	m_renderSys = (Engine::RenderSystem*)manager.GetSystem("Render");
	return true;
}

bool ComputeTest::RecompileShader()
{
	auto shader = std::make_unique<Render::ShaderProgram>();

	auto shaderBinary = Render::ShaderBinary();
	std::string cmpResult;
	bool r = shaderBinary.CompileFromFile(Render::ShaderType::ComputeShader, "compute_test.cs", cmpResult);
	if (!r || cmpResult.size() > 0)
	{
		SDE_LOG(cmpResult.c_str());
		return false;
	}
	shader->Create(shaderBinary, cmpResult);
	if (!r || cmpResult.size() > 0)
	{
		SDE_LOG(cmpResult.c_str());
		return false;
	}
	m_shader = std::move(shader);
	return true;
}

bool ComputeTest::PostInit()
{
	m_texture = std::make_unique<Render::Texture>();
	auto desc = Render::TextureSource(m_dims.x, m_dims.y, Render::TextureSource::Format::RGBAF32);
	auto clamp = Render::TextureSource::WrapMode::ClampToEdge;
	desc.SetWrapMode(clamp, clamp);
	m_texture->Create(desc);

	RecompileShader();

	return true;
}

bool ComputeTest::Tick(float timeDelta)
{
	static float s_time = 0.0f;
	s_time += timeDelta;
	if (m_texture && m_shader)
	{
		auto device = m_renderSys->GetDevice();
		device->BindShaderProgram(*m_shader);

		// uniforms should work?
		auto handle = m_shader->GetUniformHandle("Colour");
		device->SetUniformValue(handle, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
		auto t = m_shader->GetUniformHandle("Time");
		device->SetUniformValue(t, s_time);

		device->BindComputeImage(0, m_texture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly);
		device->DispatchCompute(m_dims.x, m_dims.y, 1); 

		// wait for image operations to finish
		device->MemoryBarrier(Render::BarrierType::Image);
	}

	bool keepOpen = true;
	m_debugGui->BeginWindow(keepOpen, "Compute Test", glm::vec2(m_dims));
	if (m_debugGui->Button("Reload"))
	{
		RecompileShader();
	}
	if (m_texture != nullptr)
	{
		m_debugGui->Image(*m_texture, glm::vec2(m_dims));
	}
	m_debugGui->EndWindow();
	return true;
}

void ComputeTest::Shutdown()
{
	m_texture = nullptr;
	m_shader = nullptr;
}