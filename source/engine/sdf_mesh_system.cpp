#include "sdf_mesh_system.h"
#include "core/log.h"
#include "engine/components/component_sdf_mesh.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_material.h"
#include "engine/system_manager.h"
#include "engine/debug_render.h"
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

struct VertexOutputHeader
{
	uint32_t m_dimensions[3] = { 0 };
	uint32_t m_vertexCount = 0;
};

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
	return c_dimsCubed * c_vertexSize;
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

	// Working vertex buffer
	m_workingVertexBuffer = std::make_unique<Render::RenderBuffer>();
	m_workingVertexBuffer->Create(GetWorkingDataSize(), Render::RenderBufferModification::Dynamic, true, true);

	// Compute shaders
	m_findCellVerticesShader = m_graphics->Shaders().LoadComputeShader("FindSDFVertices", "compute_test_find_vertices.cs");
	m_createTrianglesShader = m_graphics->Shaders().LoadComputeShader("CreateTriangles", "compute_test_make_triangles.cs");

	return true;
}

void SDFMeshSystem::PushSharedUniforms(Render::ShaderProgram& shader, glm::vec3 offset, glm::vec3 cellSize)
{
	auto device = m_renderSys->GetDevice();
	auto handle = shader.GetUniformHandle("WorldOffset");
	if(handle != -1)
	{
		device->SetUniformValue(handle, glm::vec4(offset,0.0f));
	}
	handle = shader.GetUniformHandle("CellSize");
	if (handle != -1)
	{
		device->SetUniformValue(handle, glm::vec4(cellSize, 0.0f));
	}
}

void SDFMeshSystem::PopulateSDF(Render::ShaderProgram& shader, glm::ivec3 dims, Render::Material* mat, glm::vec3 offset, glm::vec3 cellSize)
{
	// fill in volume data via compute
	auto device = m_renderSys->GetDevice();
	device->BindShaderProgram(shader);
	device->BindComputeImage(0, m_volumeDataTexture->GetHandle(), Render::ComputeImageFormat::RF16, Render::ComputeImageAccess::WriteOnly, true);
	if (mat)	// if we have a custom material, send the params!
	{
		Engine::ApplyMaterial(*device, shader, *mat, m_graphics->Textures());
	}
	PushSharedUniforms(shader, offset, cellSize);	// set after so the material can't screw it up!
	device->DispatchCompute(dims.x, dims.y, dims.z);
}

void SDFMeshSystem::FindVertices(Render::ShaderProgram& shader, glm::ivec3 dims, glm::vec3 offset, glm::vec3 cellSize)
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
	PushSharedUniforms(shader, offset, cellSize);
	device->BindComputeImage(0, m_vertexPositionTexture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly, true);
	device->BindComputeImage(1, m_vertexNormalTexture->GetHandle(), Render::ComputeImageFormat::RGBAF32, Render::ComputeImageAccess::WriteOnly, true);
	device->DispatchCompute(dims.x - 1, dims.y - 1, dims.z - 1);
}

void SDFMeshSystem::FindTriangles(Render::ShaderProgram& shader, glm::ivec3 dims, glm::vec3 offset, glm::vec3 cellSize)
{
	// build triangles from the vertices we found earlier

	// clear out the old header data
	VertexOutputHeader newHeader = { {(uint32_t)dims.x,(uint32_t)dims.y,(uint32_t)dims.z}, 0 };
	m_workingVertexBuffer->SetData(0, sizeof(newHeader), &newHeader);

	// ensure data is visible before image loads
	auto device = m_renderSys->GetDevice();
	device->MemoryBarrier(Render::BarrierType::Image);

	device->BindShaderProgram(shader);
	auto sampler = shader.GetUniformHandle("InputVolume");
	if (sampler != -1)
	{
		device->SetSampler(sampler, m_volumeDataTexture->GetHandle(), 0);
	}
	PushSharedUniforms(shader, offset, cellSize);
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

		// since vertices on the edges will not touch the actual bounds most of the time,
		// we add extra samples to ensure meshes can have seamless edges
		// +1 extra edge since we can't write tris for the max bounds
		const uint32_t extraLayers = 1;
		auto dims = mesh.GetResolution() + glm::ivec3(1 + extraLayers * 2);
		const glm::vec3 cellSize = (mesh.GetBoundsMax() - mesh.GetBoundsMin()) / glm::vec3(mesh.GetResolution());
		const glm::vec3 worldOffset = mesh.GetBoundsMin() -(cellSize * float(extraLayers));

		PopulateSDF(*sdfShader, dims, instanceMaterial, worldOffset, cellSize);
		FindVertices(*findVerticesShader, dims, worldOffset, cellSize);
		FindTriangles(*makeTrianglesShader, dims, worldOffset, cellSize);
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
		// eventually we should use probably buffersubdata() and only read the headers
		void* ptr = m_workingVertexBuffer->Map(Render::RenderBufferMapHint::Read, 0, GetWorkingDataSize());
		if (ptr)
		{
			VertexOutputHeader* header = (VertexOutputHeader*)ptr;
			auto vertexCount = header->m_vertexCount / 2;	// todo
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

			if (m_graphics->ShouldDrawBounds())
			{
				auto colour = glm::vec4(1, 1, 1, 1);
				m_graphics->DebugRenderer().DrawBox(m.GetBoundsMin(), m.GetBoundsMax(), colour, transform->GetMatrix());
			}
		}
	});

	return true;
}

void SDFMeshSystem::Shutdown()
{
	m_volumeDataTexture = nullptr;
	m_workingVertexBuffer = nullptr;
	m_vertexPositionTexture = nullptr;
	m_vertexNormalTexture = nullptr;
}