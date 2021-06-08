#include "render_target_blitter.h"
#include "frame_buffer.h"
#include "shader_program.h"
#include "device.h"
#include "mesh.h"
#include "mesh_builder.h"
#include "texture.h"
#include "core/profiler.h"

namespace Render
{
	RenderTargetBlitter::RenderTargetBlitter()
	{
		// screen space quad
		Render::MeshBuilder builder;
		builder.AddVertexStream(2);
		builder.BeginChunk();
		const glm::vec2 v0 = { -1.0f,-1.0f };
		const glm::vec2 v1 = { 1.0f,1.0f };
		builder.BeginTriangle();
		builder.SetStreamData(0, {v0.x,v1.y}, { v0.x,v0.y }, { v1.x,v0.y } );
		builder.EndTriangle();
		builder.BeginTriangle();
		builder.SetStreamData(0, { v0.x,v1.y }, { v1.x,v0.y } , { v1.x,v1.y } );
		builder.EndTriangle();
		builder.EndChunk();

		m_quadMesh = std::make_unique<Render::Mesh>();
		builder.CreateMesh(*m_quadMesh);
		builder.CreateVertexArray(*m_quadMesh);
	}

	RenderTargetBlitter::~RenderTargetBlitter()
	{

	}

	void RenderTargetBlitter::TargetToBackbuffer(Render::Device& d, const Render::FrameBuffer& src, Render::ShaderProgram& shader, glm::ivec2 dimensions)
	{
		SDE_PROF_EVENT();
		if (src.GetHandle() == -1 || shader.GetHandle() == -1 || src.GetColourAttachmentCount() == 0)
		{
			return;
		}
		d.DrawToBackbuffer();
		d.SetViewport(glm::ivec2(0), dimensions);
		d.BindShaderProgram(shader);
		d.BindVertexArray(m_quadMesh->GetVertexArray());
		auto sampler = shader.GetUniformHandle("SourceTexture");
		if (sampler != -1)
		{
			d.SetSampler(sampler, src.GetColourAttachment(0).GetHandle(), 0);
		}
		for (const auto& chunk : m_quadMesh->GetChunks())
		{
			d.DrawPrimitives(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount);
		}
	}

	void RenderTargetBlitter::TextureToTarget(Render::Device& d, const Render::Texture& src, Render::FrameBuffer& target, Render::ShaderProgram& shader, UniformBuffer* u)
	{
		SDE_PROF_EVENT();
		if (src.GetHandle() == -1 || target.GetHandle() == -1 || shader.GetHandle() == -1)
		{
			return;
		}
		d.DrawToFramebuffer(target);
		d.SetViewport(glm::ivec2(0), target.Dimensions());
		d.BindShaderProgram(shader);
		d.BindVertexArray(m_quadMesh->GetVertexArray());
		if (u != nullptr)
		{
			u->Apply(d, shader);
		}
		auto sampler = shader.GetUniformHandle("SourceTexture");
		if (sampler != -1)
		{
			d.SetSampler(sampler, src.GetHandle(), 0);
		}
		for (const auto& chunk : m_quadMesh->GetChunks())
		{
			d.DrawPrimitives(chunk.m_primitiveType, chunk.m_firstVertex, chunk.m_vertexCount);
		}
	}

	void RenderTargetBlitter::TargetToTarget(Render::Device& d, const Render::FrameBuffer& src, Render::FrameBuffer& target, Render::ShaderProgram& shader)
	{
		SDE_PROF_EVENT();
		TextureToTarget(d, src.GetColourAttachment(0), target, shader);
	}
}