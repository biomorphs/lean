#pragma once
#include <stdint.h>
#include "core/glm_headers.h"

namespace Render
{
	class ShaderProgram;
	class Mesh;
}

namespace Engine
{
	class Mesh;
	class ShaderProgram;
	class MeshInstance
	{
	public:
		MeshInstance() = default;
		MeshInstance(const MeshInstance&) = default;
		MeshInstance(MeshInstance&&) = default;
		MeshInstance& operator=(const MeshInstance& other) = default;
		MeshInstance& operator=(MeshInstance&& other) = default;
		glm::mat4 m_transform;
		glm::vec3 m_aabbMin;	// used in culling
		glm::vec3 m_aabbMax;
		Render::ShaderProgram* m_shader;
		const Render::Mesh* m_mesh;
		float m_distanceToCamera;
	};
}