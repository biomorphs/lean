#pragma once
#include <string>

namespace Engine
{
	class DebugGuiSystem;
}

class Scene;
class SceneEditor
{
public:
	void Init(Scene* scene, Engine::DebugGuiSystem* debugGui);
	bool Tick();
	void ToggleEnabled();

private:
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Scene* m_scene = nullptr;

	bool m_mainWindowActive = false;
	std::string m_newScriptName;
};