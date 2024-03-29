#pragma once
#include "engine/system.h"
#include "entity/entity_handle.h"
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
class TransformWidget;
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
	bool ImportScene(const char* fileName, bool makeNewWorld = false);

	const std::vector<EntityHandle>& SelectedEntities() { return m_selectedEntities; }
	void SelectEntity(const EntityHandle h);
	void DeselectEntity(const EntityHandle h);
	void DeselectAll();
	void SelectAll();

	void PushCommand(std::unique_ptr<Command>&& cmdPtr);
private:
	struct PossibleSelection
	{
		EntityHandle m_handle;
		float m_hitT;
	};
	std::vector<PossibleSelection> FindSelectionCandidates();
	void UpdateSelection();
	void DrawSelected();
	void DrawSelectionCandidates(const std::vector<PossibleSelection>&);

	void DrawGrid(float cellSize, int cellCount, glm::vec4 colour);
	void UpdateGrid();

	void UpdateUndoRedo(float timeDelta);

	std::unique_ptr<TransformWidget> m_transformWidget;

	Engine::DebugGuiSystem* m_debugGui = nullptr;
	EntitySystem* m_entitySystem = nullptr;
	CommandList m_commands;

	std::vector<EntityHandle> m_selectedEntities;
	
	std::string m_sceneName = "";
	std::string m_sceneFilepath = "";

	bool m_showCommandLists = false;
	bool m_showLightBounds = false;
	bool m_showGridSettings = false;
	float m_gridSize = 8.0f;
	int m_gridCount = 64;
	bool m_showGrid = false;
	glm::vec4 m_gridColour = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);

	void UpdateMenubar();
	bool m_keepRunning = true;
};