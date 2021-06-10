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
	size_t GetWorkingDataSize() const;
	void FindTriangles(Render::ShaderProgram& shader, glm::ivec3 dims);
	void FindVertices(Render::ShaderProgram& shader, glm::ivec3 dims);
	void PopulateSDF(Render::ShaderProgram& shader, glm::ivec3 dims, Render::Material* mat);
	void KickoffRemesh(class SDFMesh& mesh, EntityHandle handle);
	void BuildMesh();
	EntityHandle m_remeshEntity;
	const uint32_t c_maxBlockDimensions = 128;
	Render::Fence m_buildMeshFence;
	Engine::ShaderHandle m_findCellVerticesShader;
	Engine::ShaderHandle m_createTrianglesShader;
	std::unique_ptr<Render::Texture> m_volumeDataTexture;
	std::unique_ptr<Render::RenderBuffer> m_workingHeaderBuffer;
	std::unique_ptr<Render::RenderBuffer> m_workingVertexBuffer;
	std::unique_ptr<Render::Texture> m_vertexPositionTexture;
	std::unique_ptr<Render::Texture> m_vertexNormalTexture;
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::RenderSystem* m_renderSys = nullptr;
	GraphicsSystem* m_graphics = nullptr;
	EntitySystem* m_entitySystem = nullptr;
};
