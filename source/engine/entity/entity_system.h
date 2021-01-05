#pragma once
#include "engine/system.h"
#include <memory>

namespace Engine
{
	class ScriptSystem;
	class DebugGuiSystem;
}

class World;
class EntitySystem : public Engine::System
{
public:
	EntitySystem();
	virtual ~EntitySystem() = default;
	bool PreInit(Engine::SystemEnumerator&);
	bool Initialise();
	bool Tick();
	void Shutdown();

private:
	void ShowDebugGui();
	std::unique_ptr<World> m_world;
	Engine::ScriptSystem* m_scriptSystem;
	Engine::DebugGuiSystem* m_debugGui;
};