#include "component_sdf_mesh.h"
#include "engine/debug_gui_system.h"
#include "render/mesh.h"

COMPONENT_SCRIPTS(SDFMesh,
	"SetBounds", &SDFMesh::SetBounds,
	"SetResolution", &SDFMesh::SetResolution,
	"SetRenderShader", &SDFMesh::SetRenderShader,
	"SetSDFShader", &SDFMesh::SetSDFShader,
	"SetBoundsMin", &SDFMesh::SetBoundsMin,
	"SetBoundsMax", &SDFMesh::SetBoundsMax,
	"Remesh", &SDFMesh::Remesh,
	"SetMaterialEntity", &SDFMesh::SetMaterialEntity
)

COMPONENT_INSPECTOR_IMPL(SDFMesh, Engine::DebugGuiSystem& gui, Engine::TextureManager& textures)
{
	auto fn = [&gui, &textures](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<SDFMesh::StorageType&>(cs).Find(e);
		auto bMin = m.GetBoundsMin();
		auto bMax = m.GetBoundsMax();
		bMin = gui.DragVector("BoundsMin", bMin, 0.1f);
		bMax = gui.DragVector("BoundsMax", bMax, 0.1f);
		m.SetBounds(bMin, bMax);
		auto r = m.GetResolution();
		r.x = gui.DragInt("ResX", r.x, 1, 1);
		r.y = gui.DragInt("ResY", r.y, 1, 1);
		r.z = gui.DragInt("ResZ", r.z, 1, 1);
		m.SetResolution(r.x, r.y, r.z);
		if (gui.Button("Remesh Now"))
		{
			m.Remesh();
		}
	};
	return fn;
}