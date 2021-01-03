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

	class RenderTargetBlitter
	{
	public:
		RenderTargetBlitter();
		~RenderTargetBlitter() = default;

		void TargetToTarget(Render::Device& d,const Render::FrameBuffer& src, Render::FrameBuffer& target, Render::ShaderProgram& shader);
		void TargetToBackbuffer(Render::Device& d, const Render::FrameBuffer& src, Render::ShaderProgram& shader, glm::ivec2 dimensions);
	private:
		std::unique_ptr<Render::Mesh> m_quadMesh;
	};
}