#pragma once
#include "engine/system.h"
#include "engine/script_system.h"
#include "core/timer.h"
#include "scene.h"
#include "scene_editor.h"
#include <string>

namespace Engine
{
	class DebugGuiSystem;
	class ScriptSystem;
}
class EntitySystem;
class CreatureSystem;

class Playground : public Engine::System
{
public:
	Playground();
	virtual ~Playground();
	virtual bool PreInit(Engine::SystemEnumerator& systemEnumerator);
	virtual bool PostInit();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();
private:
	void NewScene();
	void LoadScene(std::string filename);
	void SaveScene(std::string filename);
	void TickScene(float timeDelta);
	void ReloadScripts();

	Scene m_scene;
	SceneEditor m_sceneEditor;
	std::vector<sol::table> m_loadedSceneScripts;
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	EntitySystem* m_entitySystem = nullptr;
	CreatureSystem* m_creatures = nullptr;
};