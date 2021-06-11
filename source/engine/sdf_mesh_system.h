#pragma once
#include "core/glm_headers.h"
#include "engine/system.h"
#include "engine/shader_manager.h"
#include "render/fence.h"
#include "entity/entity_handle.h"
#include <memory>

namespace Render
{
	class Texture;
	class RenderBuffer;
	class ShaderProgram;
	class Material;
}

namespace Engine
{
	class DebugGuiSystem;
	class RenderSystem;
}
class GraphicsSystem;
class EntitySystem;

class SDFMeshSystem : public Engine::System
{
public:
	SDFMeshSystem();
	virtual ~SDFMeshSystem();
	virtual bool PreInit(Engine::SystemManager& manager);
	virtual bool Initialise();
	virtual bool PostInit();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();

private:
	struct WorkingSet;
	void FindTriangles(WorkingSet& w, Render::ShaderProgram& shader, glm::ivec3 dims, glm::vec3 offset, glm::vec3 cellSize);
	void FindVertices(WorkingSet& w, Render::ShaderProgram& shader, glm::ivec3 dims, glm::vec3 offset, glm::vec3 cellSize);
	void PopulateSDF(WorkingSet& w, Render::ShaderProgram& shader, glm::ivec3 dims, Render::Material* mat, glm::vec3 offset, glm::vec3 cellSize);
	void KickoffRemesh(class SDFMesh& mesh, EntityHandle handle);
	void PushSharedUniforms(Render::ShaderProgram& shader, glm::vec3 offset, glm::vec3 cellSize);
	void BuildMesh(WorkingSet& w);
	size_t GetWorkingDataSize(glm::ivec3 dims) const;
	std::unique_ptr<WorkingSet> MakeWorkingSet(glm::ivec3 dimsRequired, EntityHandle h);

	const uint32_t c_maxBlockDimensions = 128;
	struct WorkingSet
	{
		WorkingSet() = default;
		WorkingSet(WorkingSet&&) = default;
		WorkingSet(const WorkingSet&) = delete;
		EntityHandle m_remeshEntity;
		Render::Fence m_buildMeshFence;
		glm::ivec3 m_dimensions;
		std::unique_ptr<Render::Texture> m_volumeDataTexture;
		std::unique_ptr<Render::RenderBuffer> m_workingVertexBuffer;
		std::unique_ptr<Render::Texture> m_vertexPositionTexture;
		std::unique_ptr<Render::Texture> m_vertexNormalTexture;
	};
	int m_maxMeshesPerFrame = 16;
	int m_maxCachedSets = 16;
	std::vector<std::unique_ptr<WorkingSet>> m_workingSetCache;
	std::vector<std::unique_ptr<WorkingSet>> m_meshesBuilding;
	Engine::ShaderHandle m_findCellVerticesShader;
	Engine::ShaderHandle m_createTrianglesShader;
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::RenderSystem* m_renderSys = nullptr;
	GraphicsSystem* m_graphics = nullptr;
	EntitySystem* m_entitySystem = nullptr;
};
