#pragma once
#include "engine/system.h"
#include "core/timer.h"
#include "scene.h"
#include "scene_editor.h"
#include <string>
#include <sol.hpp>

namespace Engine
{
	class DebugGuiSystem;
	class ScriptSystem;
}

class Playground : public Engine::System
{
public:
	Playground();
	virtual ~Playground();
	virtual bool PreInit(Engine::SystemEnumerator& systemEnumerator);
	virtual bool Tick();
	virtual void Shutdown();
private:
	void NewScene();
	void LoadScene(std::string filename);
	void SaveScene(std::string filename);
	void TickScene();
	void ReloadScripts();

	Scene m_scene;
	SceneEditor m_sceneEditor;

	std::vector<sol::table> m_loadedSceneScripts;
	std::string m_sceneFilename;

	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	Core::Timer m_timer;
	double m_lastFrameTime = 0.0;
	double m_deltaTime = 0.0;
};