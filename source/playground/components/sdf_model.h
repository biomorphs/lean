#pragma once

#include "engine/entity/component.h"
#include "core/glm_headers.h"
#include <functional>

class SDFModel
{
public:
	SDFModel() = default;
	COMPONENT(SDFModel);
	
	struct Sample { float distance = -1.0f; uint8_t material = (uint8_t)-1; };
	using SampleFn = std::function<void(float, float, float, Sample&)>;
	void SetSampleScriptFunction(sol::protected_function fn);
	void SetSampleFunction(SampleFn fn) { m_sampleFunction = fn; }
	void SetBounds(glm::vec3 minB, glm::vec3 maxB) { m_boundsMin = minB; m_boundsMax = maxB; }
	void SetResolution(int x, int y, int z) { m_meshResolution = { x,y,z }; }
	glm::vec3 GetBoundsMin() const { return m_boundsMin; }
	glm::vec3 GetBoundsMax() const { return m_boundsMax; }
	glm::ivec3 GetResolution() const { return m_meshResolution; }
	
	const SampleFn& GetSampleFn() const { return m_sampleFunction; }
	glm::vec3 SampleNormal(float x, float y, float z, float sampleDelta = 0.01f);

	void SampleCorners(glm::vec3 p0, glm::vec3 cellSize, SDFModel::Sample (&corners) [2][2][2]) const;
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
	glm::vec3 m_boundsMin = { -1.0f,-1.0f,-1.0f };
	glm::vec3 m_boundsMax = { 1.0f,1.0f,1.0f };
	glm::ivec3 m_meshResolution = {11,11,11};
	SampleFn m_sampleFunction = [](float x, float y, float z, Sample& s) {
		float sphere = glm::length(glm::vec3(x, y - 0.1f, z)) - 0.75f;
		float plane = glm::dot(glm::vec3(x, y, z), glm::vec3(0.0f, 1.0f, 0.0f)) + 1.0f;
		float opUnion = glm::min(sphere, plane);
		s.distance = opUnion;
	};
};