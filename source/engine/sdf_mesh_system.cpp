#include "sdf_mesh_system.h"
#include "core/log.h"
#include "engine/components/component_sdf_mesh.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_material.h"
#include "engine/camera_system.h"
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

struct OutputBufferHeader
{
	uint32_t m_vertexCount = 0;
	uint32_t m_dimensions[3] = { 0 };
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
	m_jobSystem = (Engine::JobSystem*)manager.GetSystem("Jobs");
	m_cameras = (Engine::CameraSystem*)manager.GetSystem("Cameras");
	
	return true;
}

bool SDFMeshSystem::Initialise()
{
	// Register component
	m_entitySystem->RegisterComponentType<SDFMesh>();
	m_entitySystem->RegisterInspector<SDFMesh>(SDFMesh::MakeInspector(*m_debugGui, m_graphics->Textures()));

	return true;
}

bool SDFMeshSystem::PostInit()
{
	SDE_PROF_EVENT();

	// Compute shaders
	m_findCellVerticesShader = m_graphics->Shaders().LoadComputeShader("FindSDFVertices", "compute_test_find_vertices.cs");
	m_createTrianglesShader = m_graphics->Shaders().LoadComputeShader("CreateTriangles", "compute_test_make_triangles.cs");

	return true;
}

uint32_t nextPowerTwo(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

std::unique_ptr<SDFMeshSystem::WorkingSet> SDFMeshSystem::MakeWorkingSet(glm::ivec3 dimsRequired, EntityHandle h)
{
	SDE_PROF_EVENT();

	for (auto it = m_workingSetCache.begin(); it != m_workingSetCache.end(); ++it)
	{
		if (glm::all(glm::greaterThanEqual((*it)->m_dimensions, dimsRequired)))
		{
			auto w = std::move(*it);
			w->m_remeshEntity = h;
			m_workingSetCache.erase(it);
			return std::move(w);
		}
	}

	auto w = std::make_unique<SDFMeshSystem::WorkingSet>();

	// The textures must be a power of two, so round them up 
	glm::ivec3 actualDims = { nextPowerTwo(dimsRequired.x), nextPowerTwo(dimsRequired.y) , nextPowerTwo(dimsRequired.z) };
	w->m_dimensions = actualDims;

	// The distance field is written to this volume texture
	auto volumedatadesc = Render::TextureSource(actualDims.x, actualDims.y, actualDims.z, Render::TextureSource::Format::RF16);
	const auto clamp = Render::TextureSource::WrapMode::ClampToEdge;
	volumedatadesc.SetWrapMode(clamp, clamp, clamp);
	w->m_volumeDataTexture = std::make_unique<Render::Texture>();
	w->m_volumeDataTexture->Create(volumedatadesc);

	// Per-cell index data written as r32ui
	auto volumeDesc = Render::TextureSource(actualDims.x, actualDims.y, actualDims.z, Render::TextureSource::Format::R32UI);
	volumeDesc.SetWrapMode(clamp, clamp, clamp);
	w->m_cellLookupTexture = std::make_unique<Render::Texture>();
	w->m_cellLookupTexture->Create(volumeDesc);

	// Working vertex + index buffer
	const auto c_dimsCubed = actualDims.x * actualDims.y * actualDims.z;
	const auto c_vertexSize = sizeof(float) * 4 * 2;
	w->m_workingVertexBuffer = std::make_unique<Render::RenderBuffer>();
	w->m_workingVertexBuffer->Create(c_dimsCubed * c_vertexSize, Render::RenderBufferModification::Dynamic, true, true);

	w->m_workingIndexBuffer = std::make_unique<Render::RenderBuffer>();
	w->m_workingIndexBuffer->Create(c_dimsCubed * sizeof(uint32_t) * 3, Render::RenderBufferModification::Dynamic, true, true);	// This may not be big enough for massive meshes

	// clear out header data early
	OutputBufferHeader newHeader = { 0, {(uint32_t)actualDims.x,(uint32_t)actualDims.y,(uint32_t)actualDims.z} };
	w->m_workingVertexBuffer->SetData(0, sizeof(newHeader), &newHeader);
	w->m_workingIndexBuffer->SetData(0, sizeof(newHeader), &newHeader);

	w->m_remeshEntity = h;

	return std::move(w);
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

// fill in volume data via compute
void SDFMeshSystem::PopulateSDF(WorkingSet& w, Render::ShaderProgram& shader, glm::ivec3 dims, Render::Material* mat, glm::vec3 offset, glm::vec3 cellSize)
{
	SDE_PROF_EVENT();

	auto device = m_renderSys->GetDevice();
	device->BindShaderProgram(shader);
	device->BindComputeImage(0, w.m_volumeDataTexture->GetHandle(), Render::ComputeImageFormat::RF16, Render::ComputeImageAccess::WriteOnly, true);
	if (mat)	// if we have a custom material, send the params!
	{
		Engine::ApplyMaterial(*device, shader, *mat, m_graphics->Textures());
	}
	PushSharedUniforms(shader, offset, cellSize);	// set after so the material can't screw it up!
	device->DispatchCompute(dims.x, dims.y, dims.z);
}

// find vertex positions + normals and indices for each cell containing an edge with a zero point
void SDFMeshSystem::FindVertices(WorkingSet& w, Render::ShaderProgram& shader, glm::ivec3 dims, glm::vec3 offset, glm::vec3 cellSize)
{
	SDE_PROF_EVENT();

	// ensure the image writes are visible before we try to fetch them via a sampler
	auto device = m_renderSys->GetDevice();
	device->MemoryBarrier(Render::BarrierType::TextureFetch);

	device->BindShaderProgram(shader);
	PushSharedUniforms(shader, offset, cellSize);
	auto sampler = shader.GetUniformHandle("InputVolume");
	if (sampler != -1)
	{
		device->SetSampler(sampler, w.m_volumeDataTexture->GetHandle(), 0);
	}
	device->BindStorageBuffer(0, *w.m_workingVertexBuffer);
	device->BindComputeImage(0, w.m_cellLookupTexture->GetHandle(), Render::ComputeImageFormat::R32UI, Render::ComputeImageAccess::WriteOnly, true);
	device->DispatchCompute(dims.x - 1, dims.y - 1, dims.z - 1);
}

// build triangles from the vertices and indices we found earlier
void SDFMeshSystem::FindTriangles(WorkingSet& w, Render::ShaderProgram& shader, glm::ivec3 dims, glm::vec3 offset, glm::vec3 cellSize)
{
	SDE_PROF_EVENT();
	
	// ensure data is visible before image loads
	auto device = m_renderSys->GetDevice();
	device->MemoryBarrier(Render::BarrierType::Image);

	device->BindShaderProgram(shader);
	PushSharedUniforms(shader, offset, cellSize);
	auto sampler = shader.GetUniformHandle("InputVolume");
	if (sampler != -1)
	{
		device->SetSampler(sampler, w.m_volumeDataTexture->GetHandle(), 0);
	}
	device->BindStorageBuffer(0, *w.m_workingIndexBuffer);
	device->BindComputeImage(0, w.m_cellLookupTexture->GetHandle(), Render::ComputeImageFormat::R32UI, Render::ComputeImageAccess::ReadOnly, true);
	device->DispatchCompute(dims.x - 5, dims.y - 5, dims.z - 5);	// -5 to account for extra layers where we dont want tris
	w.m_buildMeshFence = device->MakeFence();
}

void SDFMeshSystem::KickoffRemesh(SDFMesh& mesh, EntityHandle handle)
{
	SDE_PROF_EVENT();

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

		// vertices at the edges will get sampling artifacts due to filtering
		// we add extra samples to ensure meshes can have seamless edges
		// +1 since we can only output triangles for dims-1
		const uint32_t extraLayers = 2;
		auto dims = mesh.GetResolution() + glm::ivec3(1 + extraLayers * 2);
		const glm::vec3 cellSize = (mesh.GetBoundsMax() - mesh.GetBoundsMin()) / glm::vec3(mesh.GetResolution());
		const glm::vec3 worldOffset = mesh.GetBoundsMin() -(cellSize * float(extraLayers));

		auto w = MakeWorkingSet(dims, handle);
		OutputBufferHeader newHeader = { 0, {(uint32_t)dims.x,(uint32_t)dims.y,(uint32_t)dims.z} };
		w->m_workingVertexBuffer->SetData(0, sizeof(newHeader), &newHeader);
		w->m_workingIndexBuffer->SetData(0, sizeof(newHeader), &newHeader);

		PopulateSDF(*w, *sdfShader, dims, instanceMaterial, worldOffset, cellSize);
		FindVertices(*w, *findVerticesShader, dims, worldOffset, cellSize);
		FindTriangles(*w, *makeTrianglesShader, dims, worldOffset, cellSize);
		mesh.SetRemesh(false);
		m_meshesComputing.emplace_back(std::move(w));
		++m_meshesPending;
	}
}

void SDFMeshSystem::FinaliseMesh(WorkingSet& w)
{
	SDE_PROF_EVENT();
	if (w.m_finalMesh != nullptr)
	{
		auto world = m_entitySystem->GetWorld();
		auto meshComponent = world->GetComponent<SDFMesh>(w.m_remeshEntity);
		if (meshComponent)
		{
			w.m_finalMesh->GetVertexArray().Create();		// this is the only thing we can't do in jobs
			meshComponent->SetMesh(std::move(w.m_finalMesh));
		}
	}
	--m_meshesPending;
}

void SDFMeshSystem::BuildMeshJob(WorkingSet& w)
{
	SDE_PROF_EVENT();
	// for now copy the data to the new mesh via cpu
	// eventually we should use probably buffersubdata() and only read the headers if possible
	// it may even be worth caching + reusing entire meshes
	void* vptr = w.m_workingVertexBuffer->Map(Render::RenderBufferMapHint::Read, 0, w.m_workingVertexBuffer->GetSize());
	void* iptr = w.m_workingIndexBuffer->Map(Render::RenderBufferMapHint::Read, 0, w.m_workingIndexBuffer->GetSize());
	if (vptr && iptr)
	{
		OutputBufferHeader* vheader = (OutputBufferHeader*)vptr;
		auto vertexCount = vheader->m_vertexCount / 2;	// todo
		OutputBufferHeader* iheader = (OutputBufferHeader*)iptr;
		auto indexCount = iheader->m_vertexCount;	// todo

		if (vertexCount > 0 && indexCount > 0)
		{
			float* vbase = reinterpret_cast<float*>(vheader + 1);
			uint32_t* ibase = reinterpret_cast<uint32_t*>(iheader + 1);

			// hacks, rebuild the entire mesh
			// we dont try and change the old one since the gpu could be using it
			auto newMesh = std::make_unique<Render::Mesh>();
			newMesh->GetStreams().push_back({});
			auto& vb = newMesh->GetStreams()[0];
			vb.Create(vbase, sizeof(float) * 4 * 2 * vertexCount, Render::RenderBufferModification::Static);
			newMesh->GetVertexArray().AddBuffer(0, &vb, Render::VertexDataType::Float, 4, 0, sizeof(float) * 4 * 2);
			newMesh->GetVertexArray().AddBuffer(1, &vb, Render::VertexDataType::Float, 4, sizeof(float) * 4, sizeof(float) * 4 * 2);
			auto ib = std::make_unique<Render::RenderBuffer>();
			ib->Create(ibase, sizeof(uint32_t) * indexCount, Render::RenderBufferModification::Static);
			newMesh->GetIndexBuffer() = std::move(ib);

			newMesh->GetChunks().push_back(Render::MeshChunk(0, indexCount, Render::PrimitiveType::Triangles));
			w.m_finalMesh = std::move(newMesh);
		}
	}
	w.m_workingVertexBuffer->Unmap();
	w.m_workingIndexBuffer->Unmap();

	// Ensure any writes are shared with all contexts
	Render::Device::FlushContext();
}

void SDFMeshSystem::HandleFinishedComputeShaders()
{
	SDE_PROF_EVENT();
	auto device = m_renderSys->GetDevice();

	// Wait for all the fences in one go
	// Otherwise we fight the driver when jobs start to allocate buffers
	std::vector<uint32_t> readyForJobs;
	for (auto w = m_meshesComputing.begin(); w != m_meshesComputing.end(); ++w)
	{
		if (device->WaitOnFence((*w)->m_buildMeshFence, 10) == Render::FenceResult::Signalled)
		{
			readyForJobs.push_back(std::distance(m_meshesComputing.begin(), w));
		}
	}

	for (int i = readyForJobs.size() - 1; i >= 0; --i)
	{
		// take ownership of the working set ptr since we can't capture unique_ptr
		auto* theWorkingSet = m_meshesComputing[readyForJobs[i]].release();

		m_jobSystem->PushSlowJob([this, theWorkingSet](void*) {
			BuildMeshJob(*theWorkingSet);
			{
				Core::ScopedMutex lock(m_finaliseMeshLock);
				m_meshesToFinalise.emplace_back(std::unique_ptr<WorkingSet>(theWorkingSet));
			}
		});
		m_meshesComputing.erase(m_meshesComputing.begin() + readyForJobs[i]);
	}
}

void SDFMeshSystem::FinaliseMeshes()
{
	SDE_PROF_EVENT();

	// Finalise meshes built on jobs
	std::vector<std::unique_ptr<WorkingSet>> finaliseMeshes;
	{
		Core::ScopedMutex lock(m_finaliseMeshLock);
		finaliseMeshes = std::move(m_meshesToFinalise);
	}
	for (auto& it : finaliseMeshes)
	{
		FinaliseMesh(*it);
		if (m_workingSetCache.size() < m_maxCachedSets)
		{
			m_workingSetCache.emplace_back(std::move(it));
		}
	}
}

bool SDFMeshSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto transforms = world->GetAllComponents<Transform>();
	auto materials = world->GetAllComponents<Material>();
	auto device = m_renderSys->GetDevice();
	const auto& camera = m_cameras->MainCamera();

	// Finalise vertex arrays on main thread
	FinaliseMeshes();

	// Find meshes closest to the camera that need to be rebuilt. also submit all built meshes to the renderer
	using ClosestMesh = std::tuple<float, SDFMesh*, EntityHandle>;
	std::vector<ClosestMesh> closestMeshes;
	static robin_hood::unordered_flat_map<uint32_t, uint64_t> entityToGeneration;
	closestMeshes.reserve(m_maxComputePerFrame);
	float furthestFromCamera = FLT_MAX;
	uint32_t totalNeedBuild = 0, totalSkipped = 0;
	world->ForEachComponent<SDFMesh>([&](SDFMesh& m, EntityHandle owner) {
		const Transform* transform = transforms->Find(owner);
		if (!transform)
			return;
		if (m.NeedsRemesh())
		{
			bool isTooNew = false;
			++totalNeedBuild;
			// has it already been remeshed this generation?
			auto remeshedRecently = entityToGeneration.find(owner.GetID());
			if (remeshedRecently != entityToGeneration.end())
			{
				if (remeshedRecently->second == m_meshGeneration)
				{
					totalSkipped++;
					isTooNew = true;
				}
			}

			if (!isTooNew)
			{
				// use the midpoint of the transformed sdf bounds
				glm::vec4 midPoint = glm::vec4(m.GetBoundsMin() + (m.GetBoundsMax() - m.GetBoundsMin()) / 2.0f, 1.0f) * transform->GetMatrix();
				float distanceFromCamera = glm::distance(glm::vec3(midPoint), camera.Position());
				if (distanceFromCamera < furthestFromCamera || closestMeshes.size() < m_maxComputePerFrame)
				{
					closestMeshes.push_back({ distanceFromCamera, &m, owner });
					std::sort(closestMeshes.begin(), closestMeshes.end(), [](const ClosestMesh& c0, const ClosestMesh& c1) {
						return std::get<0>(c0) < std::get<0>(c1);
						});
					if (closestMeshes.size() > m_maxComputePerFrame)
					{
						closestMeshes.resize(m_maxComputePerFrame);
					}
					furthestFromCamera = std::get<0>(closestMeshes[closestMeshes.size() - 1]);
				}
			}
		}
		if (m.GetMesh() && m.GetRenderShader().m_index != -1)
		{
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

	if (totalNeedBuild > 0 && totalSkipped == totalNeedBuild)
	{
		entityToGeneration.clear();
		m_meshGeneration++;
	}

	for (auto& it : closestMeshes)
	{
		KickoffRemesh(*std::get<1>(it), std::get<2>(it));
		entityToGeneration.insert({std::get<2>(it).GetID(), m_meshGeneration});
	}

	// Handle finished compute shaders
	HandleFinishedComputeShaders();

	return true;
}

void SDFMeshSystem::Shutdown()
{
	SDE_PROF_EVENT();

	m_meshesToFinalise.clear();
	m_meshesComputing.clear();
	m_workingSetCache.clear();
}