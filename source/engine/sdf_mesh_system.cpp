#include "sdf_mesh_system.h"
#include "core/log.h"
#include "engine/components/component_sdf_mesh.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_material.h"
#include "engine/system_manager.h"
#include "entity/entity_system.h"
#include "engine/debug_gui_system.h"
#include "engine/render_system.h"
#include "engine/renderer.h"
#include "engine/texture_manager.h"
#include "material_helpers.h"
#include "engine/graphics_system.h"
#include "render/texture.h"
#include "render/texture_source.h"
#include "render/device.h"
#include "render/render_buffer.h"
#include "render/mesh.h"

SDFMeshSystem::SDFMeshSystem()
{
}

SDFMeshSystem::~SDFMeshSystem()
{
}

bool SDFMeshSystem::PreInit(Engine::SystemManager& manager)
{
	SDE_PROF_EVENT();

	m_debugGui = (Engine::DebugGuiSystem*)manager.GetSystem("DebugGui");
	m_renderSys = (Engine::RenderSystem*)manager.GetSystem("Render");
	m_graphics = (GraphicsSystem*)manager.GetSystem("Graphics");
	m_entitySystem = (EntitySystem*)manager.GetSystem("Entities");

	return true;
}

bool SDFMeshSystem::Initialise()
{
	// Register component
	m_entitySystem->RegisterComponentType<SDFMesh>();
	m_entitySystem->RegisterInspector<SDFMesh>(SDFMesh::MakeInspector(*m_debugGui, m_graphics->Textures()));

	return true;
}

size_t SDFMeshSystem::GetWorkingDataSize() const
{
	const auto c_dimsCubed = c_maxBlockDimensions * c_maxBlockDimensions * c_maxBlockDimensions;
	const auto c_vertexSize = sizeof(float) * 4 * 2;
	return c_dimsCubed * c_vertexSize * 3;
}

bool SDFMeshSystem::PostInit()
{
	SDE_PROF_EVENT();

	// The distance field is written to this volume texture
	auto volumedatadesc = Render::TextureSource(c_maxBlockDimensions, c_maxBlockDimensions, c_maxBlockDimensions, Render::TextureSource::Format::RF16);
	const auto clamp = Render::TextureSource::WrapMode::ClampToEdge;
	volumedatadesc.SetWrapMode(clamp, clamp, clamp);
	m_volumeDataTexture = std::make_unique<Render::Texture>();
	m_volumeDataTexture->Create(volumedatadesc);

	// Per-cell vertex data written as rgba32f
	auto volumeDesc = Render::TextureSource(c_maxBlockDimensions, c_maxBlockDimensions, c_maxBlockDimensions, Render::TextureSource::Format::RGBAF32);
	volumeDesc.SetWrapMode(clamp, clamp, clamp);
	m_vertexPositionTexture = std::make_unique<Render::Texture>();
	m_vertexPositionTexture->Create(volumeDesc);
	m_vertexNormalTexture = std::make_unique<Render::Texture>();
	m_vertexNormalTexture->Create(volumeDesc);

	// Working vertex buffer + header (stores per mesh data)
	m_workingVertexBuffer = std::make_unique<Render::RenderBuffer>();
	m_workingVertexBuffer->Create(GetWorkingDataSize(), Render::RenderBufferModification::Dynamic, true, true);
	m_workingHeaderBuffer = std::make_unique<Render::RenderBuffer>();
	m_workingHeaderBuffer->Create(sizeof(uint32_t) * 4, Render::RenderBufferModification::Dynamic, true, true);

	// Compute shaders
	m_findCellVerticesShader = m_graphics->Shaders().LoadComputeShader("FindSDFVertices", "compute_test_find_vertices.cs");
	m_createTrianglesShader = m_graphics->Shaders().LoadComputeShader("CreateTriangles", "compute_test_make_triangles.cs");

	return true;
}

void SDFMeshSystem::PopulateSDF(Render::ShaderProgram& shader, glm::ivec3 dims, Render::Material* mat)
{
	// fill in volume data via compute
	auto device = m_renderSys->GetDevice();
	device->BindShaderProgram(shader);
	device->BindComputeImage(0, m_volumeDataTexture->GetHandle(), Render::ComputeImageFormat::RF16, Render::ComputeImageAccess::WriteOnly, true);

	// if we have a custom material, send the params to the sdf compute
	if (mat)
	{
		const auto& uniforms = mat->GetUniforms();
		uniforms.Apply(*device, shader);

		const auto& samplers = mat->GetSamplers();
		int textureUnit = 0;
		for (const auto& s : samplers)
		{
			uint32_t uniformHandle = shader.GetUniformHandle(s.second.m_name.c_str());
			if (uniformHandle != -1)
			{
				Engine::TextureHandle texHandle = { s.second.m_handle };
				const auto theTexture = m_graphics->Textures().GetTexture({ texHandle });
				if (theTexture)
				{
					device->SetSampler(uniformHandle, theTexture->GetHandle(), textureUnit++);
				}
			}
		}
		Engine::ApplyMaterial(*device, shader, *mat, m_graphics->Textures(), nullptr, 0);
	}

	device->DispatchCompute(dims.x, dims.y, dims.z);
}

void SDFMeshSystem::FindVertices(Render::ShaderProgram& shader, glm::ivec3 dims)
{
	// find vertex positions + normals for each cell containing an edge with a zero point

	// ensure the image writes are visible before we try to fetch them via a sampler
	auto device = m_renderSys->GetDevice();
	device->MemoryBarrier(Render::BarrierType::TextureFetch);

	device->BindShaderProgram(shader);
	auto sampler = shader.GetUniformHandle("InputVolume");
	if (sampler != -1)
	{
		device->SetSampler(sampler, m_volumeDataTexture->GetHandle(), 0);
	}
	device->BindComputeImage(0, m_vertexPositionTexture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly, true);
	device->BindComputeImage(1, m_vertexNormalTexture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly, true);
	device->DispatchCompute(dims.x - 1, dims.y - 1, dims.z - 1);
}

void SDFMeshSystem::FindTriangles(Render::ShaderProgram& shader, glm::ivec3 dims)
{
	// build triangles from the vertices we found earlier

	// clear out the old header data
	uint32_t newCount = 0;
	m_workingVertexBuffer->SetData(0, sizeof(uint32_t), &newCount);

	// ensure data is visible before image loads
	auto device = m_renderSys->GetDevice();
	device->MemoryBarrier(Render::BarrierType::Image);

	device->BindShaderProgram(shader);
	auto sampler = shader.GetUniformHandle("InputVolume");
	if (sampler != -1)
	{
		device->SetSampler(sampler, m_volumeDataTexture->GetHandle(), 0);
	}
	device->BindComputeImage(0, m_vertexPositionTexture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::ReadOnly, true);
	device->BindComputeImage(1, m_vertexNormalTexture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::ReadOnly, true);
	device->BindStorageBuffer(0, *m_workingVertexBuffer);
	device->DispatchCompute(dims.x - 1, dims.y - 1, dims.z - 1);
	m_buildMeshFence = device->MakeFence();
}

void SDFMeshSystem::KickoffRemesh(SDFMesh& mesh, EntityHandle handle)
{
	assert(glm::all(glm::lessThan(mesh.GetResolution(), glm::ivec3(c_maxBlockDimensions))));

	auto sdfShader = m_graphics->Shaders().GetShader(mesh.GetSDFShader());
	auto findVerticesShader = m_graphics->Shaders().GetShader(m_findCellVerticesShader);
	auto makeTrianglesShader = m_graphics->Shaders().GetShader(m_createTrianglesShader);
	if (sdfShader && findVerticesShader && makeTrianglesShader)
	{
		Render::Material* instanceMaterial = nullptr;
		if (mesh.GetMaterialEntity().GetID() != -1)
		{
			auto matComponent = m_entitySystem->GetWorld()->GetComponent<Material>(mesh.GetMaterialEntity());
			if (matComponent != nullptr)
			{
				instanceMaterial = &matComponent->GetRenderMaterial();
			}
		}

		PopulateSDF(*sdfShader, mesh.GetResolution(), instanceMaterial);
		FindVertices(*findVerticesShader, mesh.GetResolution());
		FindTriangles(*makeTrianglesShader, mesh.GetResolution());
		m_remeshEntity = handle;
		mesh.SetRemesh(false);
	}
}

void SDFMeshSystem::BuildMesh()
{
	auto device = m_renderSys->GetDevice();
	auto world = m_entitySystem->GetWorld();
	auto meshComponent = world->GetComponent<SDFMesh>(m_remeshEntity);
	if (meshComponent)
	{
		// ensure writes finish before we map the buffer data for reading
		// todo, check barrier flags
		device->MemoryBarrier(Render::BarrierType::All);

		// for now copy the data to the new mesh via cpu
		// eventually we should use buffersubdata() and only read the headers
		uint32_t vertexCount = 0;
		void* ptr = m_workingVertexBuffer->Map(Render::RenderBufferMapHint::Read, 0, GetWorkingDataSize());
		if (ptr)
		{
			vertexCount = *(uint32_t*)ptr / 2;
			if (vertexCount > 0)
			{
				float* vbase = reinterpret_cast<float*>(((uint32_t*)ptr) + 4);

				// hacks, rebuild the entire mesh
				// we dont try and change the old one since the gpu could be using it
				auto newMesh = std::make_unique<Render::Mesh>();
				newMesh->GetStreams().push_back({});
				auto& vb = newMesh->GetStreams()[0];
				vb.Create(vbase, sizeof(float) * 4 * 2 * vertexCount, Render::RenderBufferModification::Static);
				newMesh->GetVertexArray().AddBuffer(0, &vb, Render::VertexDataType::Float, 4, 0, sizeof(float) * 4 * 2);
				newMesh->GetVertexArray().AddBuffer(1, &vb, Render::VertexDataType::Float, 4, sizeof(float) * 4, sizeof(float) * 4 * 2);
				newMesh->GetVertexArray().Create();
				newMesh->GetChunks().push_back(Render::MeshChunk(0, vertexCount, Render::PrimitiveType::Triangles));
				meshComponent->SetMesh(std::move(newMesh));
			}
		}
		m_workingVertexBuffer->Unmap();
	}
}

bool SDFMeshSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();
	auto materials = world->GetAllComponents<Material>();

	if (m_buildMeshFence.IsValid())
	{
		auto device = m_renderSys->GetDevice();
		if (device->WaitOnFence(m_buildMeshFence, 10) == Render::FenceResult::Signalled)
		{
			BuildMesh();
			m_remeshEntity = {};
		}
	}

	world->ForEachComponent<SDFMesh>([this, &world, &transforms, &materials](SDFMesh& m, EntityHandle owner) {
		if (!m_remeshEntity.IsValid() && !m_buildMeshFence.IsValid() && m.NeedsRemesh())
		{
			KickoffRemesh(m, owner);
		}
		if (m.GetMesh() && m.GetRenderShader().m_index != -1)
		{
			const Transform* transform = transforms->Find(owner);
			if (!transform)
				return;
			Render::Material* instanceMaterial = nullptr;
			if (m.GetMaterialEntity().GetID() != -1)
			{
				auto matComponent = materials->Find(m.GetMaterialEntity());
				if (matComponent != nullptr)
				{
					instanceMaterial = &matComponent->GetRenderMaterial();
				}
			}
			m_graphics->Renderer().SubmitInstance(transform->GetMatrix(), *m.GetMesh(), m.GetRenderShader(), m.GetBoundsMin(), m.GetBoundsMax(), instanceMaterial);
		}
	});

	return true;
}

void SDFMeshSystem::Shutdown()
{
	m_volumeDataTexture = nullptr;
	m_workingHeaderBuffer = nullptr;
	m_workingVertexBuffer = nullptr;
	m_vertexPositionTexture = nullptr;
	m_vertexNormalTexture = nullptr;
}