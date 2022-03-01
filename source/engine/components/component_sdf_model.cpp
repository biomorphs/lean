#include "component_sdf_model.h"
#include "engine/system_manager.h"
#include "engine/job_system.h"
#include "engine/debug_gui_system.h"
#include "engine/file_picker_dialog.h"
#include "render/mesh.h"
#include "render/mesh_builder.h"
#include "core/log.h"

COMPONENT_SCRIPTS(SDFModel,
	"SetBounds", &SDFModel::SetBounds,
	"SetResolution", &SDFModel::SetResolution,
	"SetSampleFunction", &SDFModel::SetSampleScriptFunction,
	"SetShader", &SDFModel::SetShader,
	"SetBoundsMin", &SDFModel::SetBoundsMin,
	"SetBoundsMax", &SDFModel::SetBoundsMax,
	"Remesh", &SDFModel::Remesh,
	"SetDebugEnabled", &SDFModel::SetDebugEnabled,
	"GetDebugEnabled", &SDFModel::GetDebugEnabled,
	"SetMeshBlocky", &SDFModel::SetMeshBlocky,
	"SetMeshSurfaceNet", &SDFModel::SetMeshSurfaceNet,
	"SetMeshDualContour", &SDFModel::SetMeshDualContour,
	"SetNormalSmoothness", &SDFModel::SetNormalSmoothness,
	"SetDiffuseTexture", &SDFModel::SetDiffuseTexture,
	"SetMaterialEntity", &SDFModel::SetMaterialEntity
)

SERIALISE_BEGIN(SDFModel)
SERIALISE_END()

// Use the common samples names as we get free default textures
const char* c_textureSamplerName = "DiffuseTexture";

void SDFModel::SetDiffuseTexture(const Engine::TextureHandle& t)
{
	m_diffuseTexture = t;
	if (m_mesh != nullptr)
	{
		m_mesh->GetMaterial().SetSampler(c_textureSamplerName, m_diffuseTexture.m_index);
	}
}

void SDFModel::UpdateMesh(Engine::JobSystem* js, Engine::SDFMeshBuilder::Debug& dbg)
{
	SDE_PROF_EVENT();
	if (m_remesh && !m_isRemeshing)
	{
		m_isRemeshing = true;
		m_remesh = false;
		const auto cellSize = (GetBoundsMax() - GetBoundsMin()) / glm::vec3(GetResolution());
		const auto meshMode = m_meshMode;
		const auto sampleFn = m_sampleFunction;
		const auto origin = GetBoundsMin();
		const auto resolution = GetResolution();
		auto buildMeshJob = [this, meshMode, sampleFn, origin, cellSize, resolution](void*)
		{
			Engine::SDFMeshBuilder builder;
			auto meshBuilder = builder.MakeMeshBuilder(meshMode, sampleFn, origin, cellSize, resolution, m_normalSmoothness>0.0f);
			{
				m_buildResults = std::move(meshBuilder);
			}
		};

		if (m_useMulticoreMeshing)
		{	
			// slow jobs since they will take multiple frames
			js->PushSlowJob(buildMeshJob);
		}
		else
		{
			buildMeshJob(nullptr);
		}
	}
	{
		SDE_PROF_EVENT("CreateMesh");
		if (m_buildResults != nullptr)
		{
			if (m_buildResults->HasData())
			{
				m_mesh = std::make_unique<Render::Mesh>();
				m_buildResults->CreateMesh(*m_mesh);
				m_buildResults->CreateVertexArray(*m_mesh);
				m_mesh->GetMaterial().SetSampler(c_textureSamplerName, m_diffuseTexture.m_index);
			}
			m_buildResults = nullptr;
			m_isRemeshing = false;
		}
	}
}

void SDFModel::SetSampleScriptFunction(sol::protected_function fn)
{
	auto wrappedFn = [fn](float x, float y, float z) -> std::tuple<float, int> {
		sol::protected_function_result result = fn(x,y,z);
		if (!result.valid())
		{
			SDE_LOG("Failed to call sample function");
		}
		std::tuple<float, int> r = result;
		return r;
	};
	m_useMulticoreMeshing = false;	//	no lua in jobs!
	m_sampleFunction = std::move(wrappedFn);
}

COMPONENT_INSPECTOR_IMPL(SDFModel, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& m = *static_cast<SDFModel::StorageType&>(cs).Find(e);
		int typeIndex = static_cast<int>(m.GetMeshMode());
		const char* types[] = { "Blocky", "Surface Net", "Dual Contour" };
		if (gui.ComboBox("Mesh Type", types, 3, typeIndex))
		{
			m.SetMeshMode(static_cast<Engine::SDFMeshBuilder::MeshMode>(typeIndex));
			m.Remesh();
		}
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
		m.SetNormalSmoothness(gui.DragFloat("Normal Smooth", m.GetNormalSmoothness(), 0.01f, 0.0f));
		auto& textures = *Engine::GetSystem<Engine::TextureManager>("Textures");
		std::string texturePath = textures.GetTexturePath(m.GetDiffuseTexture());
		texturePath = "Diffuse: " + texturePath;
		if (gui.Button(texturePath.c_str()))
		{
			std::string newFile = Engine::ShowFilePicker("Select Texture", "", "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png\0BMP (.bmp)\0*.bmp\0");
			if (newFile != "")
			{
				auto loadedTexture = textures.LoadTexture(newFile);
				m.SetDiffuseTexture(loadedTexture);
			}
		}
		if (gui.Button("Remesh Now"))
		{
			m.Remesh();
		}
		m.SetDebugEnabled(gui.Checkbox("Debug Render", m.GetDebugEnabled()));
	};
	return fn;
}