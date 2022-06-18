#pragma once
#include "render/render_pass.h"
#include "render/render_buffer.h"
#include "render/camera.h"

namespace Engine
{
	class NewRender : public Render::RenderPass
	{
	public:
		NewRender();
		void Reset();
		void RenderAll(class Render::Device&);
		void Tick(float timeDelta);
		void SetCamera(const Render::Camera& c) { m_camera = c; }

	private:
		Render::Camera m_camera;
		Render::RenderBuffer m_transforms;	// instance transforms
		uint32_t m_transformsWritten = 0;
	};
}