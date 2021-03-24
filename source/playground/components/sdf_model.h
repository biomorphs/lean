#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"
#include "engine/shader_manager.h"
#include <functional>

namespace Render
{
	class Mesh;
}

class SDFModel
{
public:
	SDFModel() = default;
	COMPONENT(SDFModel);
	
	struct Sample { 
		float distance = -1.0f; 
		uint8_t material = (uint8_t)-1; 
	};
	using SampleFn = std::function<void(float, float, float, Sample&)>;
	struct SDFDebug
	{
		virtual void DrawQuad(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {}
		virtual void DrawCellVertex(glm::vec3 p) {}
		virtual void DrawCellNormal(glm::vec3 p, glm::vec3 n) {}
	};

	enum MeshMode
	{
		Blocky,
		SurfaceNet,
		DualContour
	};
	bool GetDebugEnabled() { return m_debugRender; }
	void SetDebugEnabled(bool d) { m_debugRender = d; }
	Render::Mesh* GetMesh() const { return m_mesh.get(); }
	void Remesh() { m_remesh = true; }
	void UpdateMesh(SDFDebug& dbg = SDFDebug());
	MeshMode GetMeshMode() const { return m_meshMode; }
	void SetMeshMode(MeshMode m) { m_meshMode = m; }
	void SetShader(Engine::ShaderHandle s) { m_shader = s; }
	Engine::ShaderHandle GetShader() const { return m_shader; }
	void SetSampleScriptFunction(sol::protected_function fn);
	void SetSampleFunction(SampleFn fn) { m_sampleFunction = fn; }
	void SetBounds(glm::vec3 minB, glm::vec3 maxB) { m_boundsMin = minB; m_boundsMax = maxB; }
	void SetBoundsMin(float x, float y, float z) { m_boundsMin = { x,y,z }; }
	void SetBoundsMax(float x, float y, float z) { m_boundsMax = { x,y,z }; }
	void SetResolution(int x, int y, int z) { m_meshResolution = { x,y,z }; }
	glm::vec3 GetBoundsMin() const { return m_boundsMin; }
	glm::vec3 GetBoundsMax() const { return m_boundsMax; }
	glm::ivec3 GetResolution() const { return m_meshResolution; }
	glm::vec3 SampleNormal(float x, float y, float z, float sampleDelta = 0.01f);
	glm::vec3 GetCellSize() { return (m_boundsMax - m_boundsMin) / glm::vec3(m_meshResolution); }

	using QuadFn = std::function<void(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n0, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3)>;
	void FindQuads(const std::vector<Sample>& samples, const std::vector<glm::vec3>& v, const std::vector<glm::vec3>& n, 
		robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert, QuadFn fn);
	void FindVertices(const std::vector<Sample>& samples, SDFDebug& dbg, MeshMode mode, 
		std::vector<glm::vec3>& outV, std::vector<glm::vec3>& outN, robin_hood::unordered_map<uint64_t, uint32_t>& cellToVert);
	void SampleGrid(std::vector<Sample>& allSamples);
	void SampleCorners(glm::vec3 p0, glm::vec3 cellSize, SDFModel::Sample (&corners) [2][2][2]) const;
	void SampleCorners(int x, int y, int z, const std::vector<Sample>& v, SDFModel::Sample(&corners)[2][2][2]) const;
	bool FindVertex_Blocky(glm::vec3 p0, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3& outVertex) const;
	bool FindVertex_SurfaceNet(glm::vec3 p0, glm::vec3 cellSize, const SDFModel::Sample(&corners)[2][2][2], glm::vec3& outVertex) const;

	// surface net / dual contouring
	// Stage 1: Vertex generation
	//	for each point on a arbitrary grid
	//		sample the SDF at each corner of a 'cell' containing that point
	//		for each edge running between those corner points
	//			if the density function sign changes (+/-), generate a vertex for that cell
	//				Minecraft-style: use the cell center point
	//				Surface nets: average of all intersection points
	//				Dual contouring: solve a set of linear equations describing lines running from each point perpendicular to the normal. clamp the vertex to the cell bounds
	//				add the vertex to a list, and also keep a map of cell [x,y,z] -> vertex for later
	// Stage 2: Edge building
	// for each point on the grid
	//	for each edge in the cell (only 3 edges per point running along major axes)
	//		if this edge has a sign change between corners
	//			find the vertices from adjacent cells sharing this edge (4 of them) and make a quad
	// build mesh data from quads

private:
	bool m_remesh = true;
	bool m_debugRender = false;
	MeshMode m_meshMode = SurfaceNet;
	Engine::ShaderHandle m_shader;
	std::unique_ptr<Render::Mesh> m_mesh;
	glm::vec3 m_boundsMin = { -1.0f,-1.0f,-1.0f };
	glm::vec3 m_boundsMax = { 1.0f,1.0f,1.0f };
	glm::ivec3 m_meshResolution = {11,11,11};
	SampleFn m_sampleFunction = [](float x, float y, float z, Sample& s) {
		float sphere = glm::length(glm::vec3(x, y - 0.1f, z)) - 0.75f;
		float plane = glm::dot(glm::vec3(x, y, z), glm::vec3(0.0f, 1.0f, 0.0f)) + 1.0f;
		float opUnion = glm::min(sphere, plane);
		opUnion = glm::min(opUnion, glm::length(glm::vec3(x - 1, y - 0.1, z)) - 0.6f);
		opUnion = glm::min(opUnion, glm::length(glm::vec3(x - 1.8, y - 0.1, z)) - 0.4f);
		opUnion = glm::min(opUnion, glm::length(glm::vec3(x - 2.4, y - 0.1, z)) - 0.25f);
		opUnion = glm::min(opUnion, glm::length(glm::vec2(glm::length(glm::vec2(x, z-1.0f)) - 1.5f, y+0.5f)) - 0.2f);
		s.distance = opUnion;
	};
};