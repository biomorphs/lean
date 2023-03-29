#pragma once

#include "core/glm_headers.h"
#include <memory>

namespace Render
{
	class FrameBuffer;
	class ShaderProgram;
	class Material;
	class Device;
	class Mesh;
	class Texture;
	class UniformBuffer;

	class RenderTargetBlitter
	{
	public:
		RenderTargetBlitter();
		~RenderTargetBlitter();

		void RunOnTarget(Render::Device& d, Render::FrameBuffer& target, Render::ShaderProgram& shader, UniformBuffer* u = nullptr);
		void TextureToTarget(Render::Device& d, const Render::Texture& src, Render::FrameBuffer& target, Render::ShaderProgram& shader, UniformBuffer* u=nullptr);
		void TargetToTarget(Render::Device& d,const Render::FrameBuffer& src, Render::FrameBuffer& target, Render::ShaderProgram& shader);
		void TargetToBackbuffer(Render::Device& d, const Render::FrameBuffer& src, Render::ShaderProgram& shader, glm::ivec2 dimensions);
	private:
		std::unique_ptr<Render::Mesh> m_quadMesh;
	};
}