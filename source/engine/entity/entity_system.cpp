#include "entity_system.h"
#include "world.h"
#include "engine/system_enumerator.h"
#include "engine/debug_gui_system.h"
#include "engine/script_system.h"

#define REGISTER_COMPONENT_TYPE(className)	\
	m_world->RegisterComponentType<className>(#className);	\
	className::RegisterScripts(*m_scriptSystem);	\
	world["AddComponent_" #className] = [this](EntityHandle h) -> className* \
	{	\
		return static_cast<className*>(m_world->AddComponent(h, #className));	\
	};	\
	world["GetComponent_" #className] = [this](EntityHandle h) -> className* \
	{	\
		return static_cast<className*>(m_world->GetComponent(h, #className));	\
	};

EntitySystem::EntitySystem()
{
	m_world = std::make_unique<World>();
}

void EntitySystem::ShowDebugGui()
{
	bool isOpen = true;
	m_debugGui->BeginWindow(isOpen, "Entity System");

	if (m_debugGui->TreeNode("All Entities", true))
	{
		const auto& allEntities = m_world->AllEntities();
		for (auto entityID : allEntities)
		{
			char text[256] = "";
			sprintf_s(text, "Entity %d", entityID);
			if (m_debugGui->TreeNode(text, true))
			{
				std::vector<Component*> components = m_world->GetAllComponents(entityID);
				for (const auto& cmp : components)
				{
					if (m_debugGui->TreeNode(cmp->GetType().c_str()))
					{
						m_debugGui->TreePop();
					}
				}
				m_debugGui->TreePop();
			}
		}
		m_debugGui->TreePop();
	}
	m_debugGui->EndWindow();
}

class TestComponent : public Component
{
public:
	COMPONENT(TestComponent);
	
	std::string GetString() { return m_string; }
	void SetString(std::string s) { m_string = s; }

private:
	std::string m_string = "Testing 1 2 3";
};
COMPONENT_BEGIN(TestComponent, 
				"GetString", &TestComponent::GetString,
				"SetString", &TestComponent::SetString
)
COMPONENT_END()

class ScriptedComponent : public Component
{
public:
	COMPONENT(ScriptedComponent);
};
COMPONENT_BEGIN(ScriptedComponent)
COMPONENT_END()


bool EntitySystem::PreInit(Engine::SystemEnumerator& s)
{
	m_scriptSystem = (Engine::ScriptSystem*)s.GetSystem("Script");
	assert(m_scriptSystem);
	m_debugGui = (Engine::DebugGuiSystem*)s.GetSystem("DebugGui");
	assert(m_debugGui);

	auto& scripts = m_scriptSystem->Globals();

	// Entity Handle
	scripts.new_usertype<EntityHandle>("EntityHandle", sol::constructors<EntityHandle()>(),
		"GetID", &EntityHandle::GetID,
		"IsValid", &EntityHandle::IsValid);

	// World is a global singleton in script land for simplicity
	auto world = scripts["World"].get_or_create<sol::table>();
	world["AddEntity"] = [this]() { return m_world->AddEntity(); };

	// Component types are registered here
	// World.AddComponent_(type) and World.GetComponent_(type) are defined in lua
	REGISTER_COMPONENT_TYPE(TestComponent);
	REGISTER_COMPONENT_TYPE(ScriptedComponent);

	return true;
}

bool EntitySystem::Initialise()
{
	return true;
}

bool EntitySystem::Tick()
{
	ShowDebugGui();
	return true;
}

void EntitySystem::Shutdown()
{
	m_world = nullptr;
}