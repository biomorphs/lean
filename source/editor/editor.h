#pragma once
#include "engine/system.h"
#include "command_list.h"
#include <string>

namespace Engine
{
	class DebugGuiSystem;
}

class EntitySystem;
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
private:
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	EntitySystem* m_entitySystem = nullptr;
	CommandList m_commands;
	
	std::string m_sceneName = "";
	std::string m_sceneFilepath = "";

	void UpdateMenubar();
	bool m_keepRunning = true;
};