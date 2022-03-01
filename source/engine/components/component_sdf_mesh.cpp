#include "component_sdf_mesh.h"
#include "engine/sdf_mesh_octree.h"
#include "engine/debug_gui_system.h"
#include "render/mesh.h"

COMPONENT_SCRIPTS(SDFMesh,
	"SetBounds", &SDFMesh::SetBounds,
	"SetResolution", &SDFMesh::SetResolution,
	"SetRenderShader", &SDFMesh::SetRenderShader,
	"SetSDFShaderPath", &SDFMesh::SetSDFShaderPath,
	"Remesh", &SDFMesh::Remesh,
	"SetMaterialEntity", &SDFMesh::SetMaterialEntity,
	"SetOctreeDepth", &SDFMesh::SetOctreeDepth,
	"SetLOD", &SDFMesh::SetLOD
)

SERIALISE_BEGIN(SDFMesh)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(SDFMesh, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<SDFMesh::StorageType&>(cs).Find(e);
		auto bMin = m.GetBoundsMin();
		auto bMax = m.GetBoundsMax();
		bMin = gui.DragVector("BoundsMin", bMin, 0.1f);
		bMax = gui.DragVector("BoundsMax", bMax, 0.1f);
		if (bMin != m.GetBoundsMin() || bMax != m.GetBoundsMax())
		{
			m.SetBounds(bMin, bMax);
		}
		auto r = m.GetResolution();
		r.x = gui.DragInt("ResX", r.x, 1, 1);
		r.y = gui.DragInt("ResY", r.y, 1, 1);
		r.z = gui.DragInt("ResZ", r.z, 1, 1);
		if (r != m.GetResolution())
		{
			m.SetResolution(r.x, r.y, r.z);
		}
		auto d = m.GetOctreeDepth();
		d = gui.DragInt("Octree Depth", d, 1, 1, 8);
		if (d != m.GetOctreeDepth())
		{
			m.SetOctreeDepth(d);
		}
		if (gui.Button("Remesh Now"))
		{
			m.Remesh(true);
		}
	};
	return fn;
}

SDFMesh::SDFMesh()
{
	m_octree = std::make_unique<Engine::SDFMeshOctree>();
}

void SDFMesh::SetBounds(glm::vec3 minB, glm::vec3 maxB)
{ 
	m_boundsMin = minB; 
	m_boundsMax = maxB; 

	// we don't actually want an octree that matches the bounds exactly unless they are cubic
	auto dims = glm::abs(m_boundsMin - m_boundsMax);
	auto maxAxis = glm::compMax(dims);

	m_octree->SetBounds(minB, minB + glm::vec3(maxAxis, maxAxis, maxAxis));
	m_octree->Invalidate();
}

void SDFMesh::SetLOD(uint32_t depth, float distance)
{
	auto found = std::find_if(m_lods.begin(), m_lods.end(), [depth](const LODData& l) {
		return std::get<0>(l) == depth;
	});
	if (found == m_lods.end())
	{
		m_lods.push_back({ depth, distance });
	}
	else
	{
		*found = { depth, distance };
	}
}

void SDFMesh::SetResolution(int x, int y, int z)
{ 
	m_meshResolution = { x,y,z }; 
	m_octree->Invalidate();
}

void SDFMesh::Remesh(bool destroyAll)
{
	m_octree->Invalidate(destroyAll);
}

void SDFMesh::SetOctreeDepth(uint32_t d)
{
	assert(d < 9);
	m_octreeDepth = d;
	m_octree->SetMaxDepth(d);
	m_octree->Invalidate();
}