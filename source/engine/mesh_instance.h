#pragma once
#include <stdint.h>
#include "core/glm_headers.h"

namespace Engine
{
	class Mesh;
	class ShaderProgram;
	struct MeshInstance
	{
		glm::mat4 m_transform;
		glm::vec4 m_colour;
		Render::ShaderProgram* m_shader;
		const Render::Mesh* m_mesh;
		float m_distanceToCamera;
	};
}