#pragma once
#include "engine/system.h"
#include "engine/script_system.h"
#include "engine/tag.h"
#include "entity/entity_handle.h"
#include "core/glm_headers.h"
#include "component_creature.h"
#include <robin_hood.h>

class EntitySystem;
class GraphicsSystem;

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
	void AddScriptBehaviour(Engine::Tag tag, sol::protected_function fn);
	void AddBehaviour(Engine::Tag tag, Creature::Behaviour b);
private:
	void UpdateVision(Creature& c, glm::vec3 pos);
	EntitySystem* m_entitySystem = nullptr;
	GraphicsSystem* m_graphicsSystem = nullptr;
	Engine::ScriptSystem* m_scriptSystem = nullptr;
	using StateBehaviours = robin_hood::unordered_map<Engine::Tag, Creature::Behaviour>;
	StateBehaviours m_behaviours;
};
