#pragma once
#include "engine/system.h"
#include "core/timer.h"
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
	void ReloadScript();
	void CallScriptInit();
	void CallScriptTick();
	void CallScriptShutdown();
	sol::table m_scriptFunctions;
	Engine::DebugGuiSystem* m_debugGui = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	std::string m_scriptPath = "playground.lua";
	std::string m_scriptErrorText;
	Core::Timer m_timer;
	double m_lastFrameTime = 0.0;
	double m_deltaTime = 0.0;
};