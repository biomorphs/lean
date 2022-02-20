#include "sdf_mesh_system.h"
#include "core/log.h"
#include "engine/sdf_mesh_octree.h"
#include "engine/components/component_sdf_mesh.h"
#include "engine/components/component_transform.h"
#include "engine/components/component_material.h"
#include "engine/camera_system.h"
#include "engine/frustum.h"
#include "engine/system_manager.h"
#include "engine/debug_render.h"
#include "entity/entity_system.h"
#include "engine/debug_gui_system.h"
#include "engine/render_system.h"
#include "engine/renderer.h"
#include "engine/texture_manager.h"
#include "engine/shader_manager.h"
#include "material_helpers.h"
#include "engine/graphics_system.h"
#include "render/texture.h"
#include "render/texture_source.h"
#include "render/device.h"
#include "render/render_buffer.h"
#include "render/mesh.h"

const std::string c_writeVolumeShader = "sdf_write_volume.cs";

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

bool SDFMeshSystem::PreInit()
{
	SDE_PROF_EVENT();

	m_debugGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	m_renderSys = Engine::GetSystem<Engine::RenderSystem>("Render");
	m_graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	m_entitySystem = Engine::GetSystem<EntitySystem>("Entities");
	m_jobSystem = Engine::GetSystem<Engine::JobSystem>("Jobs");
	m_cameras = Engine::GetSystem<Engine::CameraSystem>("Cameras");
	
	return true;
}

bool SDFMeshSystem::Initialise()
{
	// Register component
	m_entitySystem->RegisterComponentType<SDFMesh>();
	m_entitySystem->RegisterInspector<SDFMesh>(SDFMesh::MakeInspector(*m_debugGui));

	return true;
}

bool SDFMeshSystem::PostInit()
{
	SDE_PROF_EVENT();

	// Compute shaders
	auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
	m_findCellVerticesShader = shaders->LoadComputeShader("FindSDFVertices", "compute_test_find_vertices.cs");
	m_createTrianglesShader = shaders->LoadComputeShader("CreateTriangles", "compute_test_make_triangles.cs");

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

std::unique_ptr<SDFMeshSystem::WorkingSet> SDFMeshSystem::MakeWorkingSet(glm::ivec3 dimsRequired, EntityHandle h, uint64_t nodeIndex)
{
	SDE_PROF_EVENT();

	for (auto it = m_workingSetCache.begin(); it != m_workingSetCache.end(); ++it)
	{
		if (glm::all(glm::greaterThanEqual((*it)->m_dimensions, dimsRequired)))
		{
			auto w = std::move(*it);
			w->m_remeshEntity = h;
			w->m_nodeIndex = nodeIndex;
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
	w->m_nodeIndex = nodeIndex;

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
		Engine::ApplyMaterial(*device, shader, *mat);
	}
	PushSharedUniforms(shader, offset, cellSize);	// set after so the material can't screw it up!
	dims = glm::vec3((dims.x + 3) & ~0x03, (dims.y + 3) & ~0x03, (dims.z + 3) & ~0x03);	// round up to next mul. 4
	device->DispatchCompute(dims.x/4, dims.y/4, dims.z/4);		
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
	dims = glm::vec3((dims.x + 3 - 1) & ~0x03, (dims.y + 3 - 1) & ~0x03, (dims.z + 3 - 1) & ~0x03);	// round up to next mul. 4
	device->DispatchCompute(dims.x / 4, dims.y / 4, dims.z / 4);
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
	dims = glm::vec3((dims.x + 3 - 5) & ~0x03, (dims.y + 3 - 5) & ~0x03, (dims.z + 3 - 5) & ~0x03);	// round up to next mul. 4
	device->DispatchCompute(dims.x / 4, dims.y / 4, dims.z / 4);
	w.m_buildMeshFence = device->MakeFence();
}

void SDFMeshSystem::KickoffRemesh(SDFMesh& mesh, EntityHandle handle, glm::vec3 boundsMin, glm::vec3 boundsMax, uint32_t depth, uint64_t nodeIndex)
{
	SDE_PROF_EVENT();
	
	Engine::ShaderManager::CustomDefines shaderDefines = { {"SDF_SHADER_INCLUDE", mesh.GetSDFShaderPath()} };
	const auto shaderName = "SDF Volume " + mesh.GetSDFShaderPath();
	auto shaders = Engine::GetSystem<Engine::ShaderManager>("Shaders");
	auto writeVolumeShader = shaders->LoadComputeShader(shaderName.c_str(), c_writeVolumeShader.c_str(), shaderDefines);

	auto sdfVolumeShader = shaders->GetShader(writeVolumeShader);
	auto findVerticesShader = shaders->GetShader(m_findCellVerticesShader);
	auto makeTrianglesShader = shaders->GetShader(m_createTrianglesShader);
	if (sdfVolumeShader && findVerticesShader && makeTrianglesShader)
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
		// we add extra samples to ensure meshes can have seamless edges for the same lod
		// for multiple lods, we just brute force add extra edges for now
		// +1 since we can only output triangles for dims-1
		const uint32_t extraLayers = 4;
		auto dims = mesh.GetResolution() + glm::ivec3(1 + extraLayers * 2);
		const glm::vec3 cellSize = (boundsMax - boundsMin) / glm::vec3(mesh.GetResolution());
		const glm::vec3 worldOffset = boundsMin -(cellSize * float(extraLayers));

		auto w = MakeWorkingSet(dims, handle, nodeIndex);
		OutputBufferHeader newHeader = { 0, {(uint32_t)dims.x,(uint32_t)dims.y,(uint32_t)dims.z} };
		w->m_workingVertexBuffer->SetData(0, sizeof(newHeader), &newHeader);
		w->m_workingIndexBuffer->SetData(0, sizeof(newHeader), &newHeader);

		PopulateSDF(*w, *sdfVolumeShader, dims, instanceMaterial, worldOffset, cellSize);
		FindVertices(*w, *findVerticesShader, dims, worldOffset, cellSize);
		FindTriangles(*w, *makeTrianglesShader, dims, worldOffset, cellSize);
		m_meshesComputing.emplace_back(std::move(w));
		++m_meshesPending;
	}
}

void SDFMeshSystem::FinaliseMesh(WorkingSet& w)
{
	SDE_PROF_EVENT();

	auto world = m_entitySystem->GetWorld();
	auto meshComponent = world->GetComponent<SDFMesh>(w.m_remeshEntity);
	if (meshComponent)
	{
		if (w.m_finalMesh != nullptr)
		{
			w.m_finalMesh->GetVertexArray().Create();		// this is the only thing we can't do in jobs
		}
		meshComponent->GetOctree().SetNodeData(w.m_nodeIndex, std::move(w.m_finalMesh));
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

			// rebuild the entire mesh
			// we dont try and change the old one since the gpu could be using it (gl lets us be lazy)
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
		if (device->WaitOnFence((*w)->m_buildMeshFence, 1) == Render::FenceResult::Signalled)
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

bool BoundsIntersect(glm::vec3 aMin, glm::vec3 aMax, glm::vec3 bMin, glm::vec3 bMax)
{
	return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
		(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
		(aMin.z <= bMax.z && aMax.z >= bMin.z);
}

float DistanceToAABB(glm::vec3 bmin, glm::vec3 bmax, glm::vec3 point)
{
	glm::vec3 c = glm::max(glm::min(point, bmax), bmin);
	return glm::distance(point,c);
}

float ScreenSpaceArea(glm::vec3 aMin, glm::vec3 aMax, glm::mat4 worldTransform, glm::mat4 viewProj)
{
	auto dims = aMax - aMin;
	glm::vec3 points[] = { 
		{aMin.x, aMin.y, aMin.z},
		{aMin.x+dims.x, aMin.y, aMin.z},
		{aMin.x, aMin.y, aMin.z+dims.z},
		{aMin.x + dims.x, aMin.y, aMin.z+dims.z},
		{aMin.x, aMin.y+dims.y, aMin.z},
		{aMin.x + dims.x + dims.y, aMin.y, aMin.z},
		{aMin.x, aMin.y + dims.y, aMin.z + dims.z},
		{aMin.x + dims.x + dims.y, aMin.y, aMin.z + dims.z},
	};
	auto minP = glm::vec4(FLT_MAX), maxP = glm::vec4 (-FLT_MAX);
	for (auto &it : points)
	{
		auto p = worldTransform * glm::vec4(it, 1.0f);
		p = p * viewProj;
		if (p.w != 0)
		{
			p = p / p.w;
		}
		minP = glm::min(p, minP);
		maxP = glm::max(p, maxP);
	}
	auto screendims = maxP - minP;
	return screendims.x * screendims.y;
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
	
	// depth/lod, distance to camera, mesh/entity, bounds, node index
	using NodeToUpdate = std::tuple<uint32_t, float, SDFMesh*, EntityHandle, glm::vec3, glm::vec3, uint64_t >;
	std::vector<NodeToUpdate> nodesToUpdate;
	nodesToUpdate.reserve(64 * 1024);

	static World::EntityIterator iterator = world->MakeIterator<SDFMesh, Transform>();
	iterator.ForEach([&](SDFMesh& m, Transform& t, EntityHandle h) {
		auto requestUpdate = [&](glm::vec3 bmin, glm::vec3 bmax, uint32_t depth, uint64_t node)
		{
			float distanceToBounds = DistanceToAABB(bmin, bmax, camera.Position());
			nodesToUpdate.push_back({ depth, distanceToBounds, &m, h, bmin, bmax, node });
		};
		auto shouldUpdateNode = [&](glm::vec3 bmin, glm::vec3 bmax, uint32_t depth)
		{
			if (glm::any(glm::greaterThan(bmin,m.GetBoundsMax())))
			{
				return false;
			}
			if (m.GetLODs().size() > 0)
			{
				float distanceToBounds = DistanceToAABB(bmin, bmax, camera.Position());
				for (const auto& lod : m.GetLODs())
				{
					// test against depth+1 to always try and load the next lod
					auto [loddepth, maxDistance] = lod;
					if (depth >= (loddepth+1) && distanceToBounds > maxDistance)
					{
						return false;
					}
				}
			}
			return true;
		};
		auto shouldDrawNode = [&](glm::vec3 bmin, glm::vec3 bmax, uint32_t depth)
		{
			if (m.GetLODs().size() > 0)
			{
				float distanceToBounds = DistanceToAABB(bmin, bmax, camera.Position());
				for (const auto& lod : m.GetLODs())
				{
					auto [loddepth, maxDistance] = lod;
					if (depth >= loddepth && distanceToBounds > maxDistance)
					{
						return false;
					}
				}
			}
			return true;
		};
		auto drawFn = [&](glm::vec3 bmin, glm::vec3 bmax, Render::Mesh& nodemesh)
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
			m_graphics->Renderer().SubmitInstance(t.GetWorldspaceMatrix(), nodemesh, m.GetRenderShader(), bmin, bmax, instanceMaterial);
			if (m_graphics->ShouldDrawBounds())
			{
				auto colour = glm::vec4(1, 0, 0, 1);
				m_graphics->DebugRenderer().DrawBox(bmin, bmax, colour, t.GetWorldspaceMatrix());
			}
		};
		if (m_graphics->ShouldDrawBounds())
		{
			auto colour = glm::vec4(1, 1, 1, 1);
			m_graphics->DebugRenderer().DrawBox(m.GetBoundsMin(), m.GetBoundsMax(), colour, t.GetWorldspaceMatrix());
		}
		m.GetOctree().Update(shouldUpdateNode, requestUpdate, shouldDrawNode, drawFn);
	});

	std::sort(nodesToUpdate.begin(), nodesToUpdate.end(), [this](const NodeToUpdate& c0, const NodeToUpdate& c1) {
		if (std::get<0>(c0) < m_maxLODUpdatePrecedence || std::get<0>(c1) < m_maxLODUpdatePrecedence)
		{
			if (std::get<0>(c0) < std::get<0>(c1))
			{
				return true;
			}
			else if (std::get<0>(c0) > std::get<0>(c1))
			{
				return false;
			}
		}
		return std::get<1>(c0) < std::get<1>(c1);
	});
	uint32_t totalNodesToUpdate = nodesToUpdate.size();
	if (nodesToUpdate.size() > m_maxComputePerFrame)
	{
		nodesToUpdate.resize(m_maxComputePerFrame);
	}
	for(const auto &it : nodesToUpdate)
	{
		auto [depth, distance, mesh, owner, bmin, bmax, node] = it;
		mesh->GetOctree().SignalNodeUpdating(node);
		KickoffRemesh(*mesh, owner, bmin, bmax, depth, node);

		if (m_graphics->ShouldDrawBounds())
		{
			auto colour = glm::vec4(1, 1, 1, 1);
			m_graphics->DebugRenderer().DrawBox(bmin, bmax, colour);
		}
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