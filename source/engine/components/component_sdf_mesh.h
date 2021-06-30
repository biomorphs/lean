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

	void Remesh();
	Engine::SDFMeshOctree& GetOctree() { return *m_octree; }

	// note that uniforms from materials will be sent to the SDF shader!
	void SetMaterialEntity(EntityHandle e) { m_materialEntity = e; }
	EntityHandle GetMaterialEntity() { return m_materialEntity; }
	void SetSDFShaderPath(std::string path) { m_sdfShaderPath = path; }
	std::string GetSDFShaderPath() { return m_sdfShaderPath; }
	void SetRenderShader(Engine::ShaderHandle s) { m_renderShader = s; }
	Engine::ShaderHandle GetRenderShader() const { return m_renderShader; }
	void SetBounds(glm::vec3 minB, glm::vec3 maxB);
	void SetResolution(int x, int y, int z);
	glm::vec3 GetBoundsMin() const { return m_boundsMin; }
	glm::vec3 GetBoundsMax() const { return m_boundsMax; }
	glm::ivec3 GetResolution() const { return m_meshResolution; }
	uint32_t GetOctreeDepth() { return m_octreeDepth; }
	void SetOctreeDepth(uint32_t d);

	using LODData = std::tuple<uint32_t, float>;	// depth, max distance
	std::vector<LODData>& GetLODs() { return m_lods; }
	void SetLOD(uint32_t depth, float distance);

private:
	std::vector<LODData> m_lods;
	std::unique_ptr<Engine::SDFMeshOctree> m_octree;
	EntityHandle m_materialEntity;
	bool m_remesh = false;
	std::string m_sdfShaderPath;
	Engine::ShaderHandle m_sdfShader;
	Engine::ShaderHandle m_renderShader;
	std::unique_ptr<Render::Mesh> m_mesh;
	uint32_t m_octreeDepth = 4;
	glm::vec3 m_boundsMin = { -1.0f,-1.0f,-1.0f };
	glm::vec3 m_boundsMax = { 1.0f,1.0f,1.0f };
	glm::ivec3 m_meshResolution = { 32,32,32 };
};