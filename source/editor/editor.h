#pragma once
#include "engine/system.h"
#include "command_list.h"
#include "core/glm_headers.h"
#include <string>
#include <set>

namespace Engine
{
	class DebugGuiSystem;
}

class EntitySystem;
class EntityHandle;
class Editor : public Engine::System
{
public:
	Editor();
	virtual ~Editor();
	virtual bool PreInit();
	virtual bool PostInit();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();
	void StopRunning();
	void NewScene(const char* sceneName);
	bool SaveScene(const char* fileName);
	bool ImportScene(const char* fileName);

	const std::set<EntityHandle>& SelectedEntities() { return m_selectedEntities; }
	void SelectEntity(const EntityHandle h);
	void DeselectEntity(const EntityHandle h);
	void DeselectAll();
	void SelectAll();
private:
	void DrawGrid(float cellSize, int cellCount, glm::vec4 colour);

	Engine::DebugGuiSystem* m_debugGui = nullptr;
	EntitySystem* m_entitySystem = nullptr;
	CommandList m_commands;

	std::set<EntityHandle> m_selectedEntities;
	
	std::string m_sceneName = "";
	std::string m_sceneFilepath = "";

	float m_gridSize = 8.0f;
	int m_gridCount = 64;
	bool m_showGrid = true;
	glm::vec4 m_gridColour = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);

	void UpdateMenubar();
	bool m_keepRunning = true;
};