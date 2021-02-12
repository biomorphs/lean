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
	struct MeshInstance
	{
		glm::mat4 m_transform;
		glm::vec4 m_colour;
		glm::vec3 m_aabbMin;	// used in culling
		glm::vec3 m_aabbMax;
		Render::ShaderProgram* m_shader;
		const Render::Mesh* m_mesh;
		float m_distanceToCamera;
	};
}