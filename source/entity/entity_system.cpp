#include "entity_system.h"
#include "world.h"
#include "engine/system_enumerator.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/script_system.h"
#include "engine/components/component_tags.h"
#include "engine/tag.h"

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

void EntitySystem::ShowDebugGui()
{
	SDE_PROF_EVENT();

	if(g_showWindow)
	{
		m_debugGui->BeginWindow(g_showWindow, "Entity System");
		if (m_debugGui->TreeNode("All Entities", true))
		{
			auto allComponentTypes = m_world->GetAllComponentTypes();
			const auto& allEntities = m_world->AllEntities();
			for (auto entityID : allEntities)
			{
				char text[1024] = "";
				sprintf_s(text, "Entity %d", entityID);
				auto tags = m_world->GetComponent<Tags>(entityID);
				if (tags && tags->AllTags().size() > 0)
				{
					char tagtext[1024] = "";
					strcat_s(text, " (");
					for (const auto& tag : tags->AllTags())
					{
						int index = &tag - tags->AllTags().data();
						sprintf_s(tagtext, "%s%s", tag.c_str(), index < tags->AllTags().size()-1 ? "," : "");
						strcat_s(text, tagtext);
					}
					strcat_s(text, ")");
				}
				if (m_debugGui->TreeNode(text))
				{
					std::vector<ComponentType> owned = m_world->GetOwnedComponentTypes(entityID);
					for (const auto& cmp : owned)
					{
						if (m_debugGui->TreeNode(cmp.c_str()))
						{
							auto uiRenderer = m_componentUiRenderers.find(cmp);
							if (uiRenderer != m_componentUiRenderers.end())
							{
								auto fn = uiRenderer->second;
								fn(*m_world->GetStorage(cmp), entityID, *m_debugGui);
							}
							m_debugGui->TreePop();
						}
					}
					std::vector<ComponentType> notOwned = allComponentTypes;
					notOwned.erase(std::remove_if(notOwned.begin(), notOwned.end(),[&owned](const ComponentType& t) {
						return std::find(owned.begin(), owned.end(), t) != owned.end();
					}), notOwned.end());
					if (notOwned.size() > 0)
					{
						int currentItem = 0;
						if (m_debugGui->ComboBox("Add Component", notOwned, currentItem))
						{
							m_world->AddComponent(entityID, notOwned[currentItem]);
						}
					}
					m_debugGui->TreePop();
				}
			}
			if (m_debugGui->Button("Add entity"))
			{
				m_world->AddEntity();
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

	// Tag
	// This is probably not a good place for it
	scripts.new_usertype<Engine::Tag>("Tag", sol::constructors<Engine::Tag(), Engine::Tag(const char*)>(),
		"AsString", &Engine::Tag::c_str,
		"GetHash", &Engine::Tag::GetHash);

	// Entity Handle
	scripts.new_usertype<EntityHandle>("EntityHandle", sol::constructors<EntityHandle()>(),
		"GetID", &EntityHandle::GetID,
		"IsValid", &EntityHandle::IsValid);

	// World is a global singleton in script land for simplicity
	auto world = scripts["World"].get_or_create<sol::table>();
	world["AddEntity"] = [this]() { return m_world->AddEntity(); };
	world["RemoveEntity"] = [this](EntityHandle e) { return m_world->RemoveEntity(e); };

	return true;
}

bool EntitySystem::Initialise()
{
	SDE_PROF_EVENT();

	auto& menu = g_entityMenu.AddSubmenu(ICON_FK_EYE " Entities");
	menu.AddItem("List", []() {	g_showWindow = true; });

	return true;
}

bool EntitySystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();
	ShowDebugGui();
	m_world->CollectGarbage();
	return true;
}

void EntitySystem::Shutdown()
{
	SDE_PROF_EVENT();
	m_world = nullptr;
}