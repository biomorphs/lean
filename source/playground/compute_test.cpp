#include "compute_test.h"
#include "core/log.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/render_system.h"
#include "engine/renderer.h"
#include "engine/graphics_system.h"
#include "render/render_target_blitter.h"
#include "render/texture.h"
#include "render/texture_source.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"
#include "render/device.h"
#include "render/frame_buffer.h"
#include "render/uniform_buffer.h"
#include "render/render_buffer.h"
#include "render/mesh.h"

// WHAT WE HAVE LEARNED
// WE ABSOLUTELY CANNOT WRITE TO A MESH BEING RENDERED
// TRIPLE BUFFER IF YOU REALLY WANT THAT
// ITS EASIER TO RECREATE THE ENTIRE MESH (BUT PROBABLY SLOW)
// WE CANNOT(!) UPDATE THE MESH BUFFER WITHOUT FULL SYNC

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

	m_graphics->Shaders().ReloadAll();
	
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

	// Create the mesh we will eventually populate from compute
	m_mesh = std::make_unique<Render::Mesh>();
	
	// Make sure to include an extra uint32_t*4 for the size to readback
	// max 3 triangles/cell
	const auto c_workingDataSize = sizeof(uint32_t) * 4 + (m_dims.x * m_dims.y * m_dims.z * sizeof(glm::vec4) * 3 * 3);
	m_workingVertexBuffer = std::make_unique<Render::RenderBuffer>();
	m_workingVertexBuffer->Create(c_workingDataSize, Render::RenderBufferModification::Dynamic, true, true);

	m_debugFrameBuffer = std::make_unique<Render::FrameBuffer>(glm::ivec2(m_dims));
	m_debugFrameBuffer->AddColourAttachment(Render::FrameBuffer::RGBA_U8);
	if (!m_debugFrameBuffer->Create())
	{
		return false;
	}

	m_blitter = std::make_unique<Render::RenderTargetBlitter>();
	m_blitShader = m_graphics->Shaders().LoadShader("3d texture slice blit", "basic_blit.vs", "basic_blit_3dslice.fs");
	m_fillVolumeShader = m_graphics->Shaders().LoadComputeShader("TestFillVolume", "compute_test_write_volume.cs");
	m_findCellVerticesShader = m_graphics->Shaders().LoadComputeShader("FindSDFVertices", "compute_test_find_vertices.cs");
	m_createTrianglesShader = m_graphics->Shaders().LoadComputeShader("CreateTriangles", "compute_test_make_triangles.cs");
	m_drawMeshShader = m_graphics->Shaders().LoadShader("SDF Basic", "sdf_model_basic.vs", "sdf_model_basic.fs");

	return true;
}

bool ComputeTest::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	static float s_time = 0.0f;
	s_time += timeDelta;

	auto device = m_renderSys->GetDevice();

	// If the fence is pending, check if the mesh is ready
	if (m_buildMeshFence.IsValid())
	{
		// Wait for the triangle builder to finish and update the mesh chunk
		if (device->WaitOnFence(m_buildMeshFence, 1000) == Render::FenceResult::Signalled)
		{
			m_buildMeshFence = {};
			// ensure writes finish before we map the buffer data for reading
			// todo, check barrier flags
			device->MemoryBarrier(Render::BarrierType::All);

			// we need to read 4 bytes at least to get the count
			// it may be a lot faster to either use persistent mapping, and/or separate the array and counter into different ssbos
			// however, for current hacks readback the entire thing and copy from cpu
			// ideally we should be using some kind of gpu buffer->buffer copy
			uint32_t vertexCount = 0;
			void* ptr = m_workingVertexBuffer->Map(Render::RenderBufferMapHint::Read, 0, (sizeof(uint32_t) * 4) + sizeof(float) * 4 * vertexCount);
			if (ptr)
			{
				vertexCount = *(uint32_t*)ptr;
				if (vertexCount > 0)
				{
					float* vbase = reinterpret_cast<float*>(((uint32_t*)ptr) + 4);

					// hacks, rebuild the entire mesh
					// we dont try and change the old one since the gpu could be using it
					m_mesh = std::make_unique<Render::Mesh>();
					m_mesh->GetStreams().push_back({});
					auto& vb = m_mesh->GetStreams()[0];
					vb.Create(vbase, sizeof(float) * 4 * vertexCount, Render::RenderBufferModification::Static);
					m_mesh->GetVertexArray().AddBuffer(0, &vb, Render::VertexDataType::Float, 4);
					m_mesh->GetVertexArray().Create();
					m_mesh->GetChunks().push_back(Render::MeshChunk(0, vertexCount, Render::PrimitiveType::Triangles));
				}
			}
			m_workingVertexBuffer->Unmap();
		}
	}
	else
	{
		// fill in volume data via compute
		auto fillVolumeShader = m_graphics->Shaders().GetShader(m_fillVolumeShader);
		if (m_volumeTexture && fillVolumeShader != nullptr)
		{
			device->BindShaderProgram(*fillVolumeShader);
			auto t = fillVolumeShader->GetUniformHandle("Time");
			if (t != -1)
			{
				device->SetUniformValue(t, s_time);
			}
			device->BindComputeImage(0, m_volumeTexture->GetHandle(), Render::ComputeImageFormat::RF16, Render::ComputeImageAccess::WriteOnly, true);
			device->DispatchCompute(m_dims.x, m_dims.y, m_dims.z);
		}

		// find vertex positions for each cell containing an edge with a zero point
		auto findCellShader = m_graphics->Shaders().GetShader(m_findCellVerticesShader);
		if (findCellShader != nullptr)
		{
			// ensure the image writes are visible before we try to fetch them via a sampler
			device->MemoryBarrier(Render::BarrierType::TextureFetch);
			device->BindShaderProgram(*findCellShader);
			auto sampler = findCellShader->GetUniformHandle("InputVolume");
			if (sampler != -1)
			{
				device->SetSampler(sampler, m_volumeTexture->GetHandle(), 0);
			}
			device->BindComputeImage(0, m_volumeTexture2->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly, true);
			device->DispatchCompute(m_dims.x - 1, m_dims.y - 1, m_dims.z - 1);
		}

		// build triangles from the vertices we found earlier
		auto makeTrianglesShader = m_graphics->Shaders().GetShader(m_createTrianglesShader);
		if (makeTrianglesShader != nullptr)
		{
			// clear out the old vertex count
			uint32_t newCount = 0;
			m_workingVertexBuffer->SetData(0, sizeof(uint32_t), &newCount);

			// ensure data is visible before image loads
			device->MemoryBarrier(Render::BarrierType::Image);
			device->BindShaderProgram(*makeTrianglesShader);
			auto sampler = makeTrianglesShader->GetUniformHandle("InputVolume");
			if (sampler != -1)
			{
				device->SetSampler(sampler, m_volumeTexture->GetHandle(), 0);
			}
			device->BindComputeImage(0, m_volumeTexture2->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::ReadOnly, true);
			device->BindStorageBuffer(1, *m_workingVertexBuffer);
			device->DispatchCompute(m_dims.x - 1, m_dims.y - 1, m_dims.z - 1);
			m_buildMeshFence = device->MakeFence();
		}
	}

	if (m_mesh->GetChunks().size() > 0)
	{
		auto transform = glm::identity<glm::mat4>();
		m_graphics->Renderer().SubmitInstance(transform, *m_mesh, m_drawMeshShader);
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
	m_workingVertexBuffer = nullptr;
	m_mesh = nullptr;
	m_blitter = nullptr;
	m_volumeTexture = nullptr;
	m_volumeTexture2 = nullptr;
	m_debugFrameBuffer = nullptr;
}