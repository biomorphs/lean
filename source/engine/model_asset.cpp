#include "model_asset.h"
#include "core/profiler.h"
#include "core/log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Engine
{
	namespace Assets
	{

		glm::mat4 ToGlMatrix(const aiMatrix4x4& m)
		{
			return {
				m.a1, m.b1, m.c1, m.d1,
				m.a2, m.b2, m.c2, m.d2,
				m.a3, m.b3, m.c3, m.d3,
				m.a4, m.b4, m.c4, m.d4,
			};
		}

		void Model::ProcessMesh(const aiScene* scene, const struct aiMesh* mesh, Model& model, glm::mat4 transform)
		{
			SDE_PROF_EVENT();

			// Make sure its triangles!
			if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
			{
				SDE_LOG("Can't handle this primitive type, sorry! Triangles only");
				return;
			}

			// Process vertices
			glm::vec3 boundsMin(FLT_MAX);
			glm::vec3 boundsMax(FLT_MIN);

			ModelMesh newMesh;
			newMesh.Transform() = transform;
			auto& vertices = newMesh.Vertices();
			vertices.reserve(mesh->mNumVertices);
			MeshVertex newVertex;
			for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
			{
				newVertex.m_position = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
				if (mesh->mNormals != nullptr)
				{
					newVertex.m_normal = glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}
				if (mesh->mTextureCoords[0] != nullptr)
				{
					newVertex.m_texCoord0 = glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
				}
				if (mesh->mTangents != nullptr)
				{
					newVertex.m_tangent = glm::vec3(mesh->mBitangents[0].x, mesh->mBitangents[0].y, mesh->mBitangents[0].z);
				}
				boundsMin = glm::min(boundsMin, newVertex.m_position);
				boundsMax = glm::max(boundsMax, newVertex.m_position);
				vertices.push_back(newVertex);
			}
			boundsMin = glm::vec3(transform * glm::vec4(boundsMin, 1.0f));
			boundsMax = glm::vec3(transform * glm::vec4(boundsMax, 1.0f));
			//newMesh.Bounds() = Math::Box3(boundsMin, boundsMax);

			// Process indices
			auto& indices = newMesh.Indices();
			indices.reserve(mesh->mNumFaces * 3);	// assuming triangles
			for (uint32_t face = 0; face < mesh->mNumFaces; ++face)
			{
				for (uint32_t faceIndex = 0; faceIndex < mesh->mFaces[face].mNumIndices; ++faceIndex)
				{
					indices.push_back(mesh->mFaces[face].mIndices[faceIndex]);
				}
			}

			// Process materials
			if (mesh->mMaterialIndex >= 0)
			{
				MeshMaterial newMaterial;
				const aiMaterial* sceneMat = scene->mMaterials[mesh->mMaterialIndex];

				// diffuse
				uint32_t diffuseTextureCount = sceneMat->GetTextureCount(aiTextureType_DIFFUSE);
				for (uint32_t t = 0; t < diffuseTextureCount; ++t)
				{
					aiString texturePath;
					sceneMat->GetTexture(aiTextureType_DIFFUSE, t, &texturePath);
					newMaterial.DiffuseMaps().push_back(texturePath.C_Str());
				}

				// normal maps
				uint32_t normalTextureCount = sceneMat->GetTextureCount(aiTextureType_NORMALS);
				for (uint32_t t = 0; t < normalTextureCount; ++t)
				{
					aiString texturePath;
					sceneMat->GetTexture(aiTextureType_NORMALS, t, &texturePath);
					newMaterial.NormalMaps().push_back(texturePath.C_Str());
				}

				// specular maps
				uint32_t specularTextureCount = sceneMat->GetTextureCount(aiTextureType_SPECULAR);
				for (uint32_t t = 0; t < specularTextureCount; ++t)
				{
					aiString texturePath;
					sceneMat->GetTexture(aiTextureType_SPECULAR, t, &texturePath);
					newMaterial.SpecularMaps().push_back(texturePath.C_Str());
				}
				aiColor3D ambientColour(0.f, 0.f, 0.f);
				aiColor3D diffuseColour(0.f, 0.f, 0.f);
				aiColor3D specularColour(0.f, 0.f, 0.f);
				float shininess = 0.0f, shininessStrength = 1.0f, opacity = 1.0f;
				sceneMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColour);
				sceneMat->Get(AI_MATKEY_COLOR_SPECULAR, specularColour);
				sceneMat->Get(AI_MATKEY_COLOR_AMBIENT, ambientColour);
				sceneMat->Get(AI_MATKEY_SHININESS, shininess);
				sceneMat->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);
				sceneMat->Get(AI_MATKEY_OPACITY, opacity);
				newMaterial.AmbientColour() = { ambientColour.r,ambientColour.g,ambientColour.b };
				newMaterial.DiffuseColour() = { diffuseColour.r, diffuseColour.g, diffuseColour.b };
				newMaterial.SpecularColour() = { specularColour.r, specularColour.g, specularColour.b };
				newMaterial.Shininess() = shininess;
				newMaterial.ShininessStrength() = shininessStrength;
				newMaterial.Opacity() = opacity;

				newMesh.SetMaterial(std::move(newMaterial));
			}

			model.Meshes().push_back(std::move(newMesh));
		}

		void Model::ParseSceneNode(const aiScene* scene, const aiNode* node, Model& model, glm::mat4 parentTransform)
		{
			glm::mat4 nodeTransform = ToGlMatrix(node->mTransformation) * parentTransform;

			for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
			{
				assert(scene->mNumMeshes >= node->mMeshes[meshIndex]);
				ProcessMesh(scene, scene->mMeshes[node->mMeshes[meshIndex]], model, nodeTransform);
			}

			for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
			{
				ParseSceneNode(scene, node->mChildren[childIndex], model, nodeTransform);
			}
		}

		std::unique_ptr<Model> Model::Load(const char* path)
		{
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "Assets::Model::Load(\"%s\")", path);
			SDE_PROF_EVENT();

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path,
				aiProcess_CalcTangentSpace |
				aiProcess_GenNormals |	// only if no normals in data
				aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices |
				aiProcess_SortByPType |
				aiProcess_PreTransformVertices
			);
			if (!scene)
			{
				return nullptr;
			}

			auto result = std::make_unique<Model>();
			result->SetPath(path);
			glm::mat4 nodeTransform = ToGlMatrix(scene->mRootNode->mTransformation);
			ParseSceneNode(scene, scene->mRootNode, *result, nodeTransform);

			return result;
		}

	}
}