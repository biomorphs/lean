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

		static std::unique_ptr<Model> CreateFromAsset(const Assets::Model& src, TextureManager& tm);

		struct Part
		{
			std::unique_ptr<Render::Mesh> m_mesh;
			glm::mat4 m_transform;
		};
		const std::vector<Part>& Parts() const { return m_parts; }
		std::vector<Part>& Parts() { return m_parts; }
	private:
		std::vector<Part> m_parts;
	};
}