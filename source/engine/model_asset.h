#pragma once

/////////////////////////////////////////////////
// BLENDER FBX EXPORT SETTINGS
// PATH MODE - RELATIVE
// Apply scalings - FBX all
// Forward -z forward
// Up Y up
// Apply unit + transformation

#include "core/glm_headers.h"
#include <vector>
#include <string>
#include <memory>

struct aiNode;
struct aiScene;
struct aiMesh;

namespace Engine
{
	namespace Assets
	{
		class MeshVertex
		{
		public:
			glm::vec3 m_position;
			glm::vec3 m_normal;
			glm::vec3 m_tangent;
			glm::vec2 m_texCoord0;
		};

		class MeshMaterial
		{
		public:
			MeshMaterial() = default;
			~MeshMaterial() = default;

			std::vector<std::string>& DiffuseMaps() { return m_diffuseMaps; }
			const std::vector<std::string>& DiffuseMaps() const { return m_diffuseMaps; }

			std::vector<std::string>& NormalMaps() { return m_normalMaps; }
			const std::vector<std::string>& NormalMaps() const { return m_normalMaps; }

			std::vector<std::string>& SpecularMaps() { return m_specularMaps; }
			const std::vector<std::string>& SpecularMaps() const { return m_specularMaps; }

			float Opacity() const { return m_opacity; }
			float& Opacity() { return m_opacity; }
			glm::vec3 DiffuseColour() const { return m_diffuseColour; }
			glm::vec3& DiffuseColour() { return m_diffuseColour; }
			glm::vec3 AmbientColour() const { return m_ambientColour; }
			glm::vec3& AmbientColour() { return m_ambientColour; }
			glm::vec3 SpecularColour() const { return m_specularColour; }
			glm::vec3& SpecularColour() { return m_specularColour; }
			float Shininess() const { return m_shininess; }
			float& Shininess() { return m_shininess; }
			float& ShininessStrength() { return m_shininessStrength; }
			float ShininessStrength() const { return m_shininessStrength; }
		private:
			std::vector<std::string> m_diffuseMaps;
			std::vector<std::string> m_normalMaps;
			std::vector<std::string> m_specularMaps;
			glm::vec3 m_diffuseColour;
			glm::vec3 m_ambientColour;
			glm::vec3 m_specularColour;
			float m_shininess;
			float m_shininessStrength;
			float m_opacity;
		};

		class ModelMesh
		{
		public:
			ModelMesh() = default;
			~ModelMesh() = default;
			ModelMesh(const ModelMesh&) = delete;
			ModelMesh(ModelMesh&&) = default;

			std::vector<MeshVertex>& Vertices() { return m_vertices; }
			const std::vector<MeshVertex>& Vertices() const { return m_vertices; }

			std::vector<uint32_t>& Indices() { return m_indices; }
			const std::vector<uint32_t>& Indices() const { return m_indices; }

			void SetMaterial(MeshMaterial&& m) { m_material = std::move(m); }
			const MeshMaterial& Material() const { return m_material; }

			glm::mat4& Transform() { return m_transform; }
			const glm::mat4& Transform() const { return m_transform; }

			glm::vec3& BoundsMin() { return m_boundsMin; }
			const glm::vec3& BoundsMin() const { return m_boundsMin; }

			glm::vec3& BoundsMax() { return m_boundsMax; }
			const glm::vec3& BoundsMax() const { return m_boundsMax; }

		private:
			std::vector<MeshVertex> m_vertices;
			std::vector<uint32_t> m_indices;
			MeshMaterial m_material;
			glm::mat4 m_transform;
			glm::vec3 m_boundsMin;
			glm::vec3 m_boundsMax;
		};

		// A render-agnostic model
		class Model
		{
		public:
			Model() = default;
			~Model() = default;
			Model(const Model&) = delete;
			Model(Model&&) = default;

			static std::unique_ptr<Model> Load(const char* path);

			std::vector<ModelMesh>& Meshes() { return m_meshes; }
			const std::vector<ModelMesh>& Meshes() const { return m_meshes; }
			const std::string& GetPath() const { return m_path; }

		private:
			static void ParseSceneNode(const aiScene* scene, const aiNode* node, Model& model, glm::mat4 parentTransform);
			static void ProcessMesh(const aiScene* scene, const aiMesh* mesh, Model& model, glm::mat4 parentTransform);
			void SetPath(const char* p) { m_path = p; }
			std::vector<ModelMesh> m_meshes;
			std::string m_path;
		};

	}
}