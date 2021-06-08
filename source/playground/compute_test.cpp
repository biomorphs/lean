#include "compute_test.h"
#include "core/log.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/render_system.h"
#include "engine/graphics_system.h"
#include "render/render_target_blitter.h"
#include "render/texture.h"
#include "render/texture_source.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"
#include "render/device.h"
#include "render/frame_buffer.h"
#include "render/uniform_buffer.h"

// TODO

// implement sdf meshing

ComputeTest::ComputeTest()
{
}

ComputeTest::~ComputeTest()
{
}

bool ComputeTest::PreInit(Engine::SystemManager & manager)
{
	SDE_PROF_EVENT();

	m_debugGui = (Engine::DebugGuiSystem*)manager.GetSystem("DebugGui");
	m_renderSys = (Engine::RenderSystem*)manager.GetSystem("Render");
	m_graphics = (GraphicsSystem*)manager.GetSystem("Graphics");
	return true;
}

bool ComputeTest::RecompileShader()
{
	SDE_PROF_EVENT();

	auto shader = std::make_unique<Render::ShaderProgram>();

	auto shaderBinary = Render::ShaderBinary();
	std::string cmpResult;
	bool r = shaderBinary.CompileFromFile(Render::ShaderType::ComputeShader, "compute_test_write_volume.cs", cmpResult);
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
	SDE_PROF_EVENT();

	// The distance field is written to this volume texture
	m_volumeTexture = std::make_unique<Render::Texture>();
	auto volumedesc = Render::TextureSource(m_dims.x, m_dims.y, m_dims.z, Render::TextureSource::Format::RF16);
	auto clamp = Render::TextureSource::WrapMode::ClampToEdge;
	volumedesc.SetWrapMode(clamp, clamp, clamp);
	m_volumeTexture->Create(volumedesc);

	// Per-cell vertex positions are written here
	m_volumeTexture2 = std::make_unique<Render::Texture>();
	auto volumedesc2 = Render::TextureSource(m_dims.x, m_dims.y, m_dims.z, Render::TextureSource::Format::RGBAF32);
	volumedesc2.SetWrapMode(clamp, clamp, clamp);
	m_volumeTexture2->Create(volumedesc2);

	m_debugFrameBuffer = std::make_unique<Render::FrameBuffer>(glm::ivec2(m_dims));
	m_debugFrameBuffer->AddColourAttachment(Render::FrameBuffer::RGBA_U8);
	if (!m_debugFrameBuffer->Create())
	{
		return false;
	}

	m_blitter = std::make_unique<Render::RenderTargetBlitter>();
	m_blitShader = m_graphics->Shaders().LoadShader("3d texture slice blit", "basic_blit.vs", "basic_blit_3dslice.fs");
	m_findCellVerticesShader = m_graphics->Shaders().LoadComputeShader("FindSDFVertices", "compute_test_find_vertices.cs");

	RecompileShader();

	return true;
}

bool ComputeTest::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	static float s_time = 0.0f;
	s_time += timeDelta;

	auto device = m_renderSys->GetDevice();

	// fill in data via compute
	if (m_volumeTexture && m_shader)
	{
		device->BindShaderProgram(*m_shader);

		// uniforms should work?
		auto t = m_shader->GetUniformHandle("Time");
		if (t != -1)
		{
			device->SetUniformValue(t, s_time);
		}
		device->BindComputeImage(0, m_volumeTexture->GetHandle(), Render::ComputeImageFormat::RF16, Render::ComputeImageAccess::WriteOnly, true);
		device->DispatchCompute(m_dims.x, m_dims.y, m_dims.z); 
	}

	auto findCellShader = m_graphics->Shaders().GetShader(m_findCellVerticesShader);
	if (findCellShader != nullptr)
	{
		// wait for image operations to finish
		device->MemoryBarrier(Render::BarrierType::Image);

		device->BindShaderProgram(*findCellShader);
		
		// pass the volume texture through a sampler (so we get 'free' filtering)
		auto sampler = findCellShader->GetUniformHandle("InputVolume");
		if (sampler != -1)
		{
			device->SetSampler(sampler, m_volumeTexture->GetHandle(), 0);
		}
		device->BindComputeImage(0, m_volumeTexture2->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly, true);
		device->DispatchCompute(m_dims.x-1, m_dims.y-1, m_dims.z-1);
	}

	bool keepOpen = true;
	m_debugGui->BeginWindow(keepOpen, "Compute Test");
	if (m_debugGui->Button("Reload"))
	{
		RecompileShader();
	}

	// debug draw a slice of the texture
	static float slice = 0;
	auto blitShader = m_graphics->Shaders().GetShader(m_blitShader);
	if (blitShader)
	{
		// wait for image operations to finish
		device->MemoryBarrier(Render::BarrierType::Image);

		// draw a slice of the texture to our frame buffer
		Render::UniformBuffer uniforms;
		uniforms.SetValue("Slice", slice);
		device->DrawToFramebuffer(*m_debugFrameBuffer);
		device->ClearFramebufferColour(*m_debugFrameBuffer, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		m_blitter->TextureToTarget(*device, *m_volumeTexture2, *m_debugFrameBuffer, *blitShader, &uniforms);
	}
	slice = m_debugGui->DragFloat("Slice", slice, 0.01f, 0.0f, 1.0f);
	m_debugGui->Image(m_debugFrameBuffer->GetColourAttachment(0), {512,512});
	m_debugGui->EndWindow();
	return true;
}

void ComputeTest::Shutdown()
{
	m_blitter = nullptr;
	m_volumeTexture = nullptr;
	m_volumeTexture2 = nullptr;
	m_shader = nullptr;
	m_debugFrameBuffer = nullptr;
}