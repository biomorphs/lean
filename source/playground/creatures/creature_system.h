#pragma once
#include "engine/system.h"
#include "engine/script_system.h"
#include "engine/tag.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include <robin_hood.h>

class EntitySystem;
class GraphicsSystem;
class Creature;

class CreatureSystem : public Engine::System
{
public:
	CreatureSystem();
	virtual ~CreatureSystem();
	virtual bool PreInit(Engine::SystemEnumerator& systemEnumerator);
	virtual bool Initialise();
	virtual bool Tick(float timeDelta);
	virtual void Shutdown();

	void Reset();

	// if a behaviour returns false, no other behaviours will be run for the current state
	using Behaviour = std::function<bool(EntityHandle, Creature&)>;
	void AddScriptBehaviour(Engine::Tag tag, sol::protected_function fn);
	void AddBehaviour(Engine::Tag tag, Behaviour b);
private:
	void UpdateVision(Creature& c, glm::vec3 pos);
	EntitySystem* m_entitySystem = nullptr;
	GraphicsSystem* m_graphicsSystem = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	using StateBehaviours = robin_hood::unordered_map<Engine::Tag, Behaviour>;
	StateBehaviours m_behaviours;
};
