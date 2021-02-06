#include "entity_system.h"
#include "world.h"
#include "engine/system_enumerator.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"

Engine::MenuBar g_entityMenu;
bool g_showWindow = false;

EntitySystem::EntitySystem()
{
	m_world = std::make_unique<World>();
}

void EntitySystem::NewWorld()
{
	SDE_PROF_EVENT();
	m_world->RemoveAllEntities();
}

void EntitySystem::RegisterComponentUi(Component::Type type, UiRenderFn fn)
{
	m_componentUiRenderers[type] = fn;
}

void EntitySystem::ShowDebugGui()
{
	SDE_PROF_EVENT();

	if(g_showWindow)
	{
		m_debugGui->BeginWindow(g_showWindow, "Entity System");
		if (m_debugGui->TreeNode("All Entities", true))
		{
			const auto& allEntities = m_world->AllEntities();
			for (auto entityID : allEntities)
			{
				char text[256] = "";
				sprintf_s(text, "Entity %d", entityID);
				if (m_debugGui->TreeNode(text))
				{
					std::vector<Component*> components = m_world->GetAllComponents(entityID);
					for (const auto& cmp : components)
					{
						if (m_debugGui->TreeNode(cmp->GetType().c_str()))
						{
							auto uiRenderer = m_componentUiRenderers.find(cmp->GetType());
							if (uiRenderer != m_componentUiRenderers.end())
							{
								auto fn = uiRenderer->second;
								fn(*cmp, *m_debugGui);
							}
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
	m_debugGui->MainMenuBar(g_entityMenu);
}

bool EntitySystem::PreInit(Engine::SystemEnumerator& s)
{
	SDE_PROF_EVENT();

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
	SDE_PROF_EVENT();

	auto& menu = g_entityMenu.AddSubmenu(ICON_FK_EYE " Entities");
	menu.AddItem("List", []() {	g_showWindow = true; });

	return true;
}

bool EntitySystem::Tick()
{
	SDE_PROF_EVENT();
	ShowDebugGui();
	return true;
}

void EntitySystem::Shutdown()
{
	SDE_PROF_EVENT();
	m_world = nullptr;
}