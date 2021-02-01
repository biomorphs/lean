#include "model.h"
#include "model_asset.h"
#include "render/mesh_builder.h"
#include "render/mesh.h"
#include "core/profiler.h"

namespace Engine
{
	std::unique_ptr<Model> Model::CreateFromAsset(const Assets::Model& m, TextureManager& tm)
	{
		char debugName[1024] = { '\0' };
		sprintf_s(debugName, "Engine::Model::CreateFromAsset(\"%s\")", m.GetPath().c_str());
		SDE_PROF_EVENT_DYN(debugName);

		auto resultModel = std::make_unique<Model>();
		resultModel->m_parts.reserve(m.Meshes().size());
		for (const auto& mesh : m.Meshes())
		{
			SDE_PROF_EVENT("BuildMesh");
			Render::MeshBuilder builder;
			builder.AddVertexStream(3, mesh.Indices().size());		// position
			builder.AddVertexStream(3, mesh.Indices().size());		// normal
			builder.AddVertexStream(3, mesh.Indices().size());		// tangents
			builder.AddVertexStream(2, mesh.Indices().size());		// uv
			builder.BeginChunk();
			const auto& vertices = mesh.Vertices();
			const auto& indices = mesh.Indices();
			{
				SDE_PROF_EVENT("SetStreamData");
				for (uint32_t index = 0; index < indices.size(); index += 3)
				{
					const auto& v0 = vertices[indices[index]];
					const auto& v1 = vertices[indices[index + 1]];
					const auto& v2 = vertices[indices[index + 2]];
					builder.BeginTriangle();
					builder.SetStreamData(0, v0.m_position, v1.m_position, v2.m_position);
					builder.SetStreamData(1, v0.m_normal, v1.m_normal, v2.m_normal);
					builder.SetStreamData(2, v0.m_tangent, v1.m_tangent, v2.m_tangent);
					builder.SetStreamData(3, v0.m_texCoord0, v1.m_texCoord0, v2.m_texCoord0);
					builder.EndTriangle();
				}
			}
			builder.EndChunk();

			auto newMesh = std::make_unique<Render::Mesh>();
			builder.CreateMesh(*newMesh);

			const auto& mat = mesh.Material();
			std::string diffusePath = mat.DiffuseMaps().size() > 0 ? mat.DiffuseMaps()[0] : "";
			std::string normalPath = mat.NormalMaps().size() > 0 ? mat.NormalMaps()[0] : "";
			std::string specPath = mat.SpecularMaps().size() > 0 ? mat.SpecularMaps()[0] : "";
			auto& material = newMesh->GetMaterial();
			material.SetSampler("DiffuseTexture", tm.LoadTexture(diffusePath.c_str()).m_index);
			material.SetSampler("NormalsTexture", tm.LoadTexture(normalPath.c_str()).m_index);
			material.SetSampler("SpecularTexture", tm.LoadTexture(specPath.c_str()).m_index);
			resultModel->m_parts.push_back({ std::move(newMesh), mesh.Transform(), mesh.BoundsMin(), mesh.BoundsMax() });
		}
		return resultModel;
	}
}