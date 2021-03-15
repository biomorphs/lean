#pragma once
#include "core/glm_headers.h"
#include "texture_manager.h"
#include <memory>

namespace Render
{
	class Mesh;
}

namespace Engine
{
	namespace Assets
	{
		class Model;
	}

	class Model
	{
	public:
		Model() = default;
		~Model() = default;

		glm::vec3& BoundsMin() { return m_boundsMin; }
		glm::vec3& BoundsMax() { return m_boundsMax; }
		const glm::vec3& BoundsMin() const { return m_boundsMin; }
		const glm::vec3& BoundsMax() const { return m_boundsMax; }
		struct Part
		{
			std::unique_ptr<Render::Mesh> m_mesh;
			glm::mat4 m_transform;
			glm::vec3 m_boundsMin;
			glm::vec3 m_boundsMax;
		};
		const std::vector<Part>& Parts() const { return m_parts; }
		std::vector<Part>& Parts() { return m_parts; }

	private:
		std::vector<Part> m_parts;
		glm::vec3 m_boundsMin = glm::vec3(FLT_MAX);
		glm::vec3 m_boundsMax = glm::vec3(-FLT_MAX);
	};
}