#pragma once

#include "entity/component.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "engine/shader_manager.h"
#include <functional>

namespace Render
{
	class Mesh;
	class TextureManager;
}
namespace Engine
{
	class TextureManager;
	class SDFMeshOctree;
}

class SDFMesh
{
public:
	SDFMesh();
	COMPONENT(SDFMesh);
	COMPONENT_INSPECTOR(Engine::DebugGuiSystem& gui, Engine::TextureManager& textures);

	void SetRemesh(bool m) { m_remesh = m; }
	void Remesh() { m_remesh = true; }
	bool NeedsRemesh() { return m_remesh; }
	const Render::Mesh* GetMesh() const { return m_mesh.get(); }
	void SetMesh(std::unique_ptr<Render::Mesh>&& m) { m_mesh = std::move(m); }

	Engine::SDFMeshOctree& GetOctree() { return *m_octree; }

	// params
	// note that uniforms from materials will be sent to the SDF shader!
	void SetMaterialEntity(EntityHandle e) { m_materialEntity = e; }
	EntityHandle GetMaterialEntity() { return m_materialEntity; }
	void SetSDFShader(Engine::ShaderHandle s) { m_sdfShader = s; }
	Engine::ShaderHandle GetSDFShader() const { return m_sdfShader; }
	void SetRenderShader(Engine::ShaderHandle s) { m_renderShader = s; }
	Engine::ShaderHandle GetRenderShader() const { return m_renderShader; }
	void SetBounds(glm::vec3 minB, glm::vec3 maxB);
	void SetResolution(int x, int y, int z);
	glm::vec3 GetBoundsMin() const { return m_boundsMin; }
	glm::vec3 GetBoundsMax() const { return m_boundsMax; }
	glm::ivec3 GetResolution() const { return m_meshResolution; }

private:
	std::unique_ptr<Engine::SDFMeshOctree> m_octree;
	EntityHandle m_materialEntity;
	bool m_remesh = false;
	Engine::ShaderHandle m_sdfShader;
	Engine::ShaderHandle m_renderShader;
	std::unique_ptr<Render::Mesh> m_mesh;
	glm::vec3 m_boundsMin = { -1.0f,-1.0f,-1.0f };
	glm::vec3 m_boundsMax = { 1.0f,1.0f,1.0f };
	glm::ivec3 m_meshResolution = { 32,32,32 };
};