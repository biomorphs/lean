#include "sdf_model.h"
#include "engine/job_system.h"
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
	"SetDiffuseTexture", &SDFModel::SetDiffuseTexture
)

// Use the common samples names as we get free default textures
const char* c_textureSamplerName = "DiffuseTexture";

void SDFModel::SetDiffuseTexture(Engine::TextureHandle t)
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