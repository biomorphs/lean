#pragma once

#include "entity/component.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "engine/shader_manager.h"
#include "engine/sdf_mesh_builder.h"
#include "engine/texture_manager.h"
#include <functional>

namespace Render
{
	class Mesh;
}
namespace Engine
{
	class JobSystem;
}

class SDFModel
{
public:
	SDFModel() = default;
	COMPONENT(SDFModel);

	void UpdateMesh(Engine::JobSystem* js, Engine::SDFMeshBuilder::Debug& dbg = Engine::SDFMeshBuilder::Debug());
	Render::Mesh* GetMesh() const { return m_mesh.get(); }

	// params
	void SetMaterialEntity(EntityHandle e) { m_materialEntity = e; }
	EntityHandle GetMaterialEntity() { return m_materialEntity; }

	bool IsRemeshing() { return m_isRemeshing; }
	void SetShader(Engine::ShaderHandle s) { m_shader = s; }
	Engine::ShaderHandle GetShader() const { return m_shader; }
	void SetMeshBlocky() { m_meshMode = Engine::SDFMeshBuilder::Blocky; }
	void SetMeshSurfaceNet() { m_meshMode = Engine::SDFMeshBuilder::SurfaceNet; }
	void SetMeshDualContour() { m_meshMode = Engine::SDFMeshBuilder::DualContour; }
	Engine::SDFMeshBuilder::MeshMode GetMeshMode() const { return m_meshMode; }
	void SetMeshMode(Engine::SDFMeshBuilder::MeshMode m) { m_meshMode = m; }
	void SetBounds(glm::vec3 minB, glm::vec3 maxB) { m_boundsMin = minB; m_boundsMax = maxB; }
	void SetBoundsMin(float x, float y, float z) { m_boundsMin = { x,y,z }; }
	void SetBoundsMax(float x, float y, float z) { m_boundsMax = { x,y,z }; }
	void SetResolution(int x, int y, int z) { m_meshResolution = { x,y,z }; }
	glm::vec3 GetBoundsMin() const { return m_boundsMin; }
	glm::vec3 GetBoundsMax() const { return m_boundsMax; }
	glm::ivec3 GetResolution() const { return m_meshResolution; }
	bool GetDebugEnabled() { return m_debugRender; }
	void SetDebugEnabled(bool d) { m_debugRender = d; }
	float GetNormalSmoothness() { return m_normalSmoothness; }
	void SetNormalSmoothness(float s) { m_normalSmoothness = s; }
	void SetDiffuseTexture(Engine::TextureHandle t);
	Engine::TextureHandle GetDiffuseTexture() const { return m_diffuseTexture; }

	// sdf data provider
	void SetSampleScriptFunction(sol::protected_function fn);
	void SetSampleFunction(Engine::SDF::SampleFn fn) { m_sampleFunction = fn; }
	const Engine::SDF::SampleFn& GetSampleFn() const { return m_sampleFunction; }

	// meshing stuff (probably refactor out)
	void Remesh() { m_remesh = true; }
	bool NeedsRemesh() { return m_remesh; }

private:
	EntityHandle m_materialEntity;
	Engine::TextureHandle m_diffuseTexture = Engine::TextureHandle::Invalid();
	bool m_useMulticoreMeshing = true;
	bool m_isRemeshing = false;
	bool m_remesh = false;
	bool m_debugRender = false;
	Engine::SDFMeshBuilder::MeshMode m_meshMode = Engine::SDFMeshBuilder::SurfaceNet;
	Engine::ShaderHandle m_shader;
	std::unique_ptr<Render::Mesh> m_mesh;
	float m_normalSmoothness = 1.0f;	// adjusts the sample distance when calculating normals
	glm::vec3 m_boundsMin = { -1.0f,-1.0f,-1.0f };
	glm::vec3 m_boundsMax = { 1.0f,1.0f,1.0f };
	glm::ivec3 m_meshResolution = {11,11,11};
	std::unique_ptr<Render::MeshBuilder> m_buildResults;
	Engine::SDF::SampleFn m_sampleFunction = [](float x, float y, float z) -> std::tuple<float, int> {

		auto Sphere = [](glm::vec3 p, float r) -> float
		{
			return glm::length(p) - r;
		};
		auto Plane = [](glm::vec3 p, glm::vec3 n, float h) -> float
		{
			return glm::dot(p, n) + h;
		};
		auto Union = [](float p0, float p1)
		{
			return glm::min(p0, p1);
		};
		auto Subtract = [](float d1, float d2)
		{
			return glm::max(-d1, d2);
		};
		auto Repeat = [](glm::vec3 p, glm::vec3 c)
		{
			return glm::mod(p + c * 0.5f, c) - c * 0.5f;
		};
		auto RidgeNoise = [](glm::vec2 p) {
			return 2.0f * (0.5f - fabs(0.5f - glm::simplex(p)));
		};

		//float dd = glm::simplex(glm::vec3(27445 + x * 0.0025f, 5166 + y * 0.0025f, 14166 + z * 0.0025f)) * 4.0f +
		//	glm::simplex(glm::vec3(x * 0.05f, y * 0.05f, z * 0.05f)) * 2.0f +
		//	glm::simplex(glm::vec3(x * 0.1f, y * 0.1f, z * 0.1f)) * 1.0f +
		//	glm::simplex(glm::vec3(x + 23 * 0.4f, y - 12 * 0.4f, z + 14 * 0.5f)) * 0.5f +
		//	glm::simplex(glm::vec3(x + 71 * 1.0f, y * 1.0f, z * 1.0f) * 0.25f);
		//float d = 1.0f - dd;

		float terrainNoise = RidgeNoise(glm::vec2(12.3f + x * 0.01f, 51.2f + z * 0.01f)) * 1.0f +
							 RidgeNoise(glm::vec2(40.1f + x * 0.02f, 27.2f + z * 0.02f)) * 0.5f + 
							 RidgeNoise(glm::vec2(97.4f + x * 0.04f, 64.2f + z * 0.04f)) * 0.25f + 
							 RidgeNoise(glm::vec2(13.1f + x * 0.08f, 89.2f + z * 0.08f)) * 0.125f + 
							 RidgeNoise(glm::vec2(76.3f + x * 0.16f, 12.2f + z * 0.16f)) * 0.0625f;
		terrainNoise = terrainNoise / (1.0f + 0.5f +	0.25f +	0.125f + 0.0625f);
		float d = y - 16 + terrainNoise * 48;
		d = Subtract(glm::simplex(glm::vec3(x * 0.0005f,y * 0.0005f,z * 0.0005f) * 64.0f) - terrainNoise, d);
		d = Union(d,Plane({ x,y,z }, { 0.0f,1.0f,0.0f }, 0.0f));

		//float d = Plane({ x,y,z }, { 0.0f,1.0f,0.0f }, 0.4f);
		//d = Union(d, Sphere(Repeat({ x,y,z }, { 8.0f,8.0f,8.0f }), 2.0f));

		return std::make_tuple(d, 0);
	};
};