#include "2d_render_context.h"
#include "engine/system_manager.h"
#include "render/frame_buffer.h"

namespace Engine
{
	struct VertexData
	{
		glm::vec2 m_position;
		glm::vec2 m_uv;
		glm::vec4 m_colour;
	};

	struct PerTriangleData
	{
		uint64_t m_texture;
	};

	void RenderContext2D::Reset(glm::vec2 dimensions)
	{
		SDE_PROF_EVENT();

		if (m_viewportDimensions != dimensions)
		{
			Initialise(dimensions);
			m_viewportDimensions = dimensions;
		}
		m_thisFrameTriangles.clear();
		m_thisFrameTriangles.reserve(1024 * 32);
		m_currentBuffer++;
		if (m_currentBuffer >= m_vertices.size())
		{
			m_currentBuffer = 0;
		}
	}

	void RenderContext2D::DrawTriangle(const glm::vec2 pos[3], int zIndex, const glm::vec2 uv[3], const glm::vec4 colour[3], TextureHandle t)
	{
		Triangle newTriangle;
		newTriangle.m_position[0] = pos[0];
		newTriangle.m_position[1] = pos[1];
		newTriangle.m_position[2] = pos[2];
		newTriangle.m_uv[0] = uv[0];
		newTriangle.m_uv[1] = uv[1];
		newTriangle.m_uv[2] = uv[2];
		newTriangle.m_colour[0] = colour[0];
		newTriangle.m_colour[1] = colour[1];
		newTriangle.m_colour[2] = colour[2];
		newTriangle.m_texture = t;
		newTriangle.m_zIndex = zIndex;
		m_thisFrameTriangles.emplace_back(newTriangle);
	}

	void RenderContext2D::DrawLine(glm::vec2 p0, glm::vec2 p1, float width, int zIndex, glm::vec4 c0, glm::vec4 c1)
	{
		glm::vec2 direction = glm::normalize(p1 - p0);
		glm::vec2 perpendicularCCW = { -direction.y, direction.x };	// quick 2d perpendicular trick

		const glm::vec2 t0array[3] = {
			p0 + perpendicularCCW * (width/2),
			p1 + perpendicularCCW * (width / 2),
			p0 - perpendicularCCW * (width / 2)
		};
		const glm::vec2 t1array[3] = {
			p0 - perpendicularCCW * (width / 2),
			p1 + perpendicularCCW * (width / 2),
			p1 - perpendicularCCW * (width / 2),
		};
		const glm::vec2 uv0array[3] = {
			{0,1},
			{1,1},
			{0,0}
		};
		const glm::vec2 uv1array[3] = {
			{0,0},
			{1,1},
			{1,0}
		};
		const glm::vec4 c0array[3] = {
			c0, 
			c1, 
			c0
		};
		const glm::vec4 c1array[3] = {
			c0,
			c1,
			c1
		};
		DrawTriangle(t0array, zIndex, uv0array, c0array, m_whiteTexture);
		DrawTriangle(t1array, zIndex, uv1array, c1array, m_whiteTexture);
	}

	void RenderContext2D::DrawQuad(glm::vec2 origin, int zIndex, glm::vec2 dimensions, glm::vec2 uv0, glm::vec2 uv1, glm::vec4 colour, TextureHandle texture)
	{
		const glm::vec4 col[3] = {
			colour, colour, colour
		};
		const glm::vec2 t0array[3] = {
			origin, 
			{origin.x + dimensions.x, origin.y},
			{origin.x + dimensions.x, origin.y + dimensions.y}
		};
		const glm::vec2 uv0array[3] = {
			uv0,
			{uv1.x, uv0.y},
			{uv1.x, uv1.y},
		};
		const glm::vec2 t1array[3] = {
			origin,
			{origin.x + dimensions.x, origin.y + dimensions.y},
			{origin.x, origin.y + dimensions.y}
		};
		const glm::vec2 uv1array[3] = {
			uv0,
			uv1,
			{uv0.x, uv1.y},
		};
		DrawTriangle(t0array, zIndex, uv0array, col, texture);
		DrawTriangle(t1array, zIndex, uv1array, col, texture);
	}

	void RenderContext2D::Render(Render::Device& d)
	{
		SDE_PROF_EVENT();

		std::sort(m_thisFrameTriangles.begin(), m_thisFrameTriangles.end(), [](const Triangle& t0, const Triangle& t1) {
			return t0.m_zIndex < t1.m_zIndex;
		});

		if (m_thisFrameTriangles.size() > 0 && m_vertices.size() > 0 && m_currentBuffer < m_vertices.size())
		{
			// dump triangles to buffers
			auto textures = Engine::GetSystem<TextureManager>("Textures");
			static std::vector<VertexData> verts;
			static std::vector<PerTriangleData> triData;
			verts.reserve(m_maxVertices);
			triData.reserve(m_maxVertices);
			verts.clear();
			triData.clear();
			for (const auto& t : m_thisFrameTriangles)
			{
				Render::Texture* texture = textures->GetTexture(t.m_texture);
				if (texture)
				{
					triData.emplace_back(PerTriangleData{ texture->GetResidentHandle() });
					verts.emplace_back(VertexData{ t.m_position[0], t.m_uv[0], t.m_colour[0] });
					verts.emplace_back(VertexData{ t.m_position[1], t.m_uv[1], t.m_colour[1] });
					verts.emplace_back(VertexData{ t.m_position[2], t.m_uv[2], t.m_colour[2] });
				}
			}
			m_vertices[m_currentBuffer]->SetData(0, verts.size() * sizeof(VertexData), verts.data());
			m_triangleData[m_currentBuffer]->SetData(0, triData.size() * sizeof(PerTriangleData), triData.data());

			// draw
			auto shader = Engine::GetSystem<ShaderManager>("Shaders")->GetShader(m_shader);
			if (shader)
			{
				d.BindShaderProgram(*shader);
				d.BindVertexArray(*m_vertexArrays[m_currentBuffer]);
				d.SetViewport(glm::ivec2(0, 0), glm::ivec2(m_viewportDimensions));
				d.SetWireframeDrawing(false);
				d.SetBackfaceCulling(false, true);
				d.SetBlending(true);
				d.SetScissorEnabled(false);
				d.SetDepthState(false, false);

				glm::mat4 projection = glm::ortho(0.0f, m_viewportDimensions.x, 0.0f, m_viewportDimensions.y, -100.0f, 100.0f);
				uint32_t projUniformHandle = shader->GetUniformHandle("ProjectionMat");
				d.SetUniformValue(projUniformHandle, projection);
				d.BindStorageBuffer(0, *m_triangleData[m_currentBuffer]);

				d.DrawPrimitives(Render::PrimitiveType::Triangles, 0, m_thisFrameTriangles.size() * 3);
			}
		}
	}

	void RenderContext2D::Initialise(glm::vec2 dimensions)
	{
		SDE_PROF_EVENT();

		if (m_vertices.size() == 0)
		{
			for (int i = 0; i < m_maxBuffers; ++i)
			{
				auto tris = std::make_unique<Render::RenderBuffer>();
				tris->Create(sizeof(PerTriangleData) * m_maxVertices, Render::RenderBufferModification::Dynamic);

				auto vertices = std::make_unique<Render::RenderBuffer>();
				vertices->Create(sizeof(VertexData) * m_maxVertices, Render::RenderBufferModification::Dynamic);

				auto va = std::make_unique<Render::VertexArray>();
				va->AddBuffer(0, vertices.get(), Render::VertexDataType::Float, 2, 0, sizeof(VertexData));
				va->AddBuffer(1, vertices.get(), Render::VertexDataType::Float, 2, sizeof(glm::vec2), sizeof(VertexData));
				va->AddBuffer(2, vertices.get(), Render::VertexDataType::Float, 4, sizeof(glm::vec2) + sizeof(glm::vec2), sizeof(VertexData));
				va->Create();

				m_triangleData.emplace_back(std::move(tris));
				m_vertices.emplace_back(std::move(vertices));
				m_vertexArrays.emplace_back(std::move(va));
			}
		}

		ShaderManager* sm = Engine::GetSystem<ShaderManager>("Shaders");
		m_shader = sm->LoadShader("RenderContext2D", "engine/shaders/render_context_2d.vs", "engine/shaders/render_context_2d.fs");

		TextureManager* tm = Engine::GetSystem<TextureManager>("Textures");
		m_whiteTexture = tm->LoadTexture("white.bmp");
	}

	void RenderContext2D::Shutdown()
	{
		SDE_PROF_EVENT();
		m_vertices.clear();
	}
}