#include "debug_render.h"
#include "renderer.h"
#include "frustum.h"
#include "render/render_buffer.h"
#include "render/mesh.h"
#include "core/log.h"
#include "core/profiler.h"
#include "shader_manager.h"
#include "system_manager.h"
#include <cassert>

namespace Engine
{
	DebugRender::DebugRender()
	{
		auto deleter = [](glm::vec4* p)
		{
			_aligned_free(p);
		};

		void* rawBuffer = _aligned_malloc(c_maxLines * sizeof(glm::vec4) * 2, 16);
		if (rawBuffer != nullptr)
		{
			memset(rawBuffer, 0, c_maxLines * sizeof(glm::vec4) * 2);
			m_posBuffer = std::unique_ptr<glm::vec4[], decltype(deleter)>((glm::vec4*)rawBuffer, deleter);
		}
		rawBuffer = _aligned_malloc(c_maxLines * sizeof(glm::vec4) * 2, 16);
		if (rawBuffer != nullptr)
		{
			memset(rawBuffer, 0, c_maxLines * sizeof(glm::vec4) * 2);
			m_colBuffer = std::unique_ptr<glm::vec4[], decltype(deleter)>((glm::vec4*)rawBuffer, deleter);
		}

		m_shader = Engine::GetSystem<ShaderManager>("Shaders")->LoadShader("DebugRender", "DebugRender.vs", "DebugRender.fs");

		CreateMesh();
	}

	void DebugRender::PushToRenderer(Renderer& r)
	{
		SDE_PROF_EVENT();

		// update current, draw current-1
		auto lineCount = m_currentLines;
		PushLinesToMesh(*m_renderMesh[m_currentWriteMesh]);
		auto meshToDraw = (m_currentWriteMesh - 1) % c_meshBuffers;
		if (lineCount > 0)
		{
			r.SubmitInstance(glm::identity<glm::mat4>(), *m_renderMesh[meshToDraw], m_shader);
		}
		m_currentWriteMesh = (m_currentWriteMesh + 1) % c_meshBuffers;
	}

	bool DebugRender::CreateMesh()
	{
		SDE_PROF_EVENT();
		for (uint32_t i = 0; i < c_meshBuffers; ++i)
		{
			auto& mesh = m_renderMesh[i];
			mesh = std::make_unique<Render::Mesh>();
			auto& streams = mesh->GetStreams();
			auto& vertexArray = mesh->GetVertexArray();
			auto& chunks = mesh->GetChunks();
			mesh->GetMaterial().SetIsTransparent(true);

			// init vertex streams and vertex array
			streams.resize(2);
			Render::RenderBuffer& posBuffer = streams[0], & colourBuffer = streams[1];
			if (!posBuffer.Create(c_maxLines * sizeof(glm::vec4) * 2, Render::RenderBufferModification::Dynamic, true))
			{
				SDE_LOGC(SDE, "Failed to create debug pos buffer");
				return false;
			}
			if (!colourBuffer.Create(c_maxLines * sizeof(glm::vec4) * 2, Render::RenderBufferModification::Dynamic, true))
			{
				SDE_LOGC(SDE, "Failed to create debug colour buffer");
				return false;
			}
			vertexArray.AddBuffer(0, &streams[0], Render::VertexDataType::Float, 4);
			vertexArray.AddBuffer(1, &streams[1], Render::VertexDataType::Float, 4);
			if (!vertexArray.Create())
			{
				SDE_LOGC(SDE, "Failed to create debug vertex array");
				return false;
			}

			// add a chunk for each primitive type
			chunks.push_back(Render::MeshChunk(0, 0, Render::PrimitiveType::Lines));
		}
		return true;
	}

	void DebugRender::AddLinesInternal(const __m128* posBuffer, const __m128* colBuffer, uint32_t count)
	{
		uint32_t toAdd = count;
		if ((m_currentLines + count) > c_maxLines)
		{
			toAdd = c_maxLines - m_currentLines;
		}

		if (count > 0)
		{
			glm::vec4* posData = m_posBuffer.get() + (m_currentLines * 2);
			glm::vec4* colData = m_colBuffer.get() + (m_currentLines * 2);
			memcpy(posData, posBuffer, toAdd * sizeof(glm::vec4) * 2);
			memcpy(colData, colBuffer, toAdd * sizeof(glm::vec4) * 2);
			m_currentLines += toAdd;
		}
		assert(m_currentLines < c_maxLines);
	}

	void DebugRender::AddLines(const glm::vec4* v, const glm::vec4* c, uint32_t count)
	{
		uint32_t toAdd = count;
		if ((m_currentLines + count) > c_maxLines)
		{
			toAdd = c_maxLines - m_currentLines;
		}

		if (count > 0)
		{
			glm::vec4* posData = m_posBuffer.get() + (m_currentLines * 2);
			glm::vec4* colData = m_colBuffer.get() + (m_currentLines * 2);
			memcpy(posData, v, toAdd * sizeof(glm::vec4) * 2);
			memcpy(colData, c, toAdd * sizeof(glm::vec4) * 2);
			m_currentLines += toAdd;
		}
		assert(m_currentLines < c_maxLines);
	}

	void DebugRender::AddAxisAtPoint(const glm::vec4& point, float scale)
	{
		const __m128 c_xAxis = { scale, 0.0f, 0.0f, 0.0f };
		const __m128 c_yAxis = { 0.0f, scale, 0.0f, 0.0f };
		const __m128 c_zAxis = { 0.0f, 0.0f, scale, 0.0f };
		__declspec(align(16)) glm::vec4 pointAligned = point;
		const __m128 c_point = _mm_load_ps(glm::value_ptr(pointAligned));
		const __m128 c_xColour = { 1.0f, 0.0f, 0.0f, 1.0f };
		const __m128 c_yColour = { 0.0f, 1.0f, 0.0f, 1.0f };
		const __m128 c_zColour = { 0.0f, 0.0f, 1.0f, 1.0f };
		__declspec(align(16)) const __m128 c_colours[] = {
			c_xColour, c_xColour,
			c_yColour, c_yColour,
			c_zColour, c_zColour
		};

		__declspec(align(16)) __m128 positions[] = {
			c_point, _mm_add_ps(c_point, c_xAxis),
			c_point, _mm_add_ps(c_point, c_yAxis),
			c_point, _mm_add_ps(c_point, c_zAxis),
		};

		AddLinesInternal(positions, c_colours, 3);
	}

	void DebugRender::AddBox(glm::vec3 center, glm::vec3 dimensions, glm::vec4 colour)
	{
		glm::vec3 minv = center - (0.5f * dimensions);
		glm::vec3 maxv = center + (0.5f * dimensions);
		const __m128 c_colour = { colour.r, colour.g, colour.b, colour.a };
		__declspec(align(16)) const __m128 colours[] = {
			c_colour, c_colour, c_colour, c_colour, c_colour, c_colour, c_colour, c_colour,
			c_colour, c_colour, c_colour, c_colour, c_colour, c_colour, c_colour, c_colour,
			c_colour, c_colour, c_colour, c_colour, c_colour, c_colour, c_colour, c_colour,
		};

		__declspec(align(16)) __m128 positions[] = {
			{minv.x, minv.y, minv.z}, {maxv.x, minv.y, minv.z},		// -x,-y,-z -> x,-y,-y
			{maxv.x, minv.y, minv.z}, {maxv.x, maxv.y, minv.z},		//  x,-y,-z -> x,y,-z
			{minv.x, maxv.y, minv.z}, {maxv.x, maxv.y, minv.z},		//  -x,y,-z -> x,y,-z
			{minv.x, minv.y, minv.z}, {minv.x, maxv.y, minv.z},		//  -x,-y,-z -> -x,y,-z

			{minv.x, minv.y, maxv.z}, {maxv.x, minv.y, maxv.z},		// -x,-y,z -> x,-y,z
			{maxv.x, minv.y, maxv.z}, {maxv.x, maxv.y, maxv.z},		//  x,-y,z -> x,y,z
			{minv.x, maxv.y, maxv.z}, {maxv.x, maxv.y, maxv.z},		//  -x,y,z -> x,y,z
			{minv.x, minv.y, maxv.z}, {minv.x, maxv.y, maxv.z},		//  -x,-y,z -> -x,y,z

			{minv.x, minv.y, minv.z}, {minv.x, minv.y, maxv.z},		//	-x,-y,-z -> -x,-y,z
			{minv.x, maxv.y, minv.z}, {minv.x, maxv.y, maxv.z},		//	-x,y,-z -> -x,y,z
			{maxv.x, minv.y, minv.z}, {maxv.x, minv.y, maxv.z},		//	x,-y,-z -> x,-y,z
			{maxv.x, maxv.y, minv.z}, {maxv.x, maxv.y, maxv.z},		//	x,y,-z -> x,y,z
			
		};

		AddLinesInternal(positions, colours, 12);
	}

	void DebugRender::PushLinesToMesh(Render::Mesh& target)
	{
		SDE_PROF_EVENT();

		auto& theChunk = target.GetChunks()[0];
		auto& posStream = target.GetStreams()[0];
		auto& colourStream = target.GetStreams()[1];

		// push data to gpu
		posStream.SetData(0, m_currentLines * sizeof(glm::vec4) * 2, m_posBuffer.get());
		colourStream.SetData(0, m_currentLines * sizeof(glm::vec4) * 2, m_colBuffer.get());

		// update chunk
		theChunk.m_firstVertex = 0;
		theChunk.m_vertexCount = m_currentLines * 2;

		// remove old lines
		m_currentLines = 0;
	}

	void DebugRender::DrawLine(glm::vec3 p0, glm::vec3 p1, glm::vec3 colour)
	{
		glm::vec4 v[] = {
			{p0,1.0f},	{p1,1.0f},
		};
		const glm::vec4 c[] = {
			{colour,1.0f},{colour,1.0f}
		};
		AddLines(v, c, 1);
	}

	void DebugRender::DrawBox(glm::vec3 bmin, glm::vec3 bmax, glm::vec4 colour, glm::mat4 boxTransform)
	{
		glm::vec4 v[] = {
			{bmin.x,bmin.y,bmin.z,1.0f},	{bmax.x,bmin.y,bmin.z,1.0f},
			{bmax.x,bmin.y,bmin.z,1.0f},	{bmax.x,bmin.y,bmax.z,1.0f},
			{bmax.x,bmin.y,bmax.z,1.0f},	{bmin.x,bmin.y,bmax.z,1.0f},
			{bmin.x,bmin.y,bmax.z,1.0f},	{bmin.x,bmin.y,bmin.z,1.0f},
			{bmin.x,bmax.y,bmin.z,1.0f},	{bmax.x,bmax.y,bmin.z,1.0f},
			{bmax.x,bmax.y,bmin.z,1.0f},	{bmax.x,bmax.y,bmax.z,1.0f},
			{bmax.x,bmax.y,bmax.z,1.0f},	{bmin.x,bmax.y,bmax.z,1.0f},
			{bmin.x,bmax.y,bmax.z,1.0f},	{bmin.x,bmax.y,bmin.z,1.0f},
			{bmin.x,bmin.y,bmin.z,1.0f},	{bmin.x,bmax.y,bmin.z,1.0f},
			{bmax.x,bmin.y,bmin.z,1.0f},	{bmax.x,bmax.y,bmin.z,1.0f},
			{bmax.x,bmin.y,bmax.z,1.0f},	{bmax.x,bmax.y,bmax.z,1.0f},
			{bmin.x,bmin.y,bmax.z,1.0f},	{bmin.x,bmax.y,bmax.z,1.0f},
		};

		for (auto& vert : v)
		{
			vert = boxTransform * vert;
		}

		const glm::vec4 c[] = {
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
		};
		AddLines(v, c, 12);
	}

	void DebugRender::DrawFrustum(const class Frustum& f, glm::vec4 colour)
	{
		const auto& p = f.GetPoints();
		glm::vec4 v[] = {
				{p[0],1.0f},	{p[1],1.0f},	// lbn,	ltn
				{p[1],1.0f},	{p[3],1.0f},	// ltn, rtn
				{p[3],1.0f},	{p[2],1.0f},	// rtn, rbn
				{p[2],1.0f},	{p[0],1.0f},	// rbn, lbn
				{p[0],1.0f},	{p[4],1.0f},	// lbn, lbf
				{p[1],1.0f},	{p[5],1.0f},	// ltn, ltf
				{p[2],1.0f},	{p[6],1.0f},	// rbn, rbf
				{p[3],1.0f},	{p[7],1.0f},	// rtn, rtf
				{p[4],1.0f},	{p[5],1.0f},	// lbf,	ltf
				{p[5],1.0f},	{p[7],1.0f},	// ltf, rtf
				{p[7],1.0f},	{p[6],1.0f},	// rtf, rbf
				{p[6],1.0f},	{p[4],1.0f},	// rbf, lbf
		};
		const glm::vec4 c[] = {
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
			colour,colour,
		};
		AddLines(v, c, 24);
	}
}