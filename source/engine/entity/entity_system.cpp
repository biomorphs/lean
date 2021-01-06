#include "entity_system.h"
#include "world.h"
#include "engine/system_enumerator.h"
#include "engine/debug_gui_system.h"
#include "engine/script_system.h"

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