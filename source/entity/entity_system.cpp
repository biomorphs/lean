#include "entity_system.h"
#include "world.h"
#include "engine/system_manager.h"
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

std::string EntitySystem::GetEntityNameWithTags(EntityHandle e) const
{
	char text[1024] = "";
	sprintf_s(text, "Entity %d", e.GetID());
	auto tags = m_world->GetComponent<Tags>(e);
	if (tags && tags->AllTags().size() > 0)
	{
		char tagtext[1024] = "";
		strcat_s(text, " (");
		for (const auto& tag : tags->AllTags())
		{
			int index = &tag - tags->AllTags().data();
			sprintf_s(tagtext, "%s%s", tag.c_str(), index < tags->AllTags().size() - 1 ? "," : "");
			strcat_s(text, tagtext);
		}
		strcat_s(text, ")");
	}
	return text;
}

void EntitySystem::ShowDebugGui()
{
	SDE_PROF_EVENT();
	static std::string filterText = "";

	if(g_showWindow)
	{
		m_debugGui->BeginWindow(g_showWindow, "Entity System");
		m_debugGui->TextInput("Tag Filter", filterText);
		if (m_debugGui->TreeNode("All Entities", true))
		{
			auto allComponentTypes = m_world->GetAllComponentTypes();
			const auto& allEntities = m_world->AllEntities();
			for (auto entityID : allEntities)
			{
				auto tags = m_world->GetComponent<Tags>(entityID);
				if (filterText.size() > 0 && tags != nullptr)
				{
					bool foundMatching = false;
					for (const auto& tag : tags->AllTags())
					{
						if (strstr(tag.c_str(), filterText.c_str()) != nullptr)
						{
							foundMatching = true;
							break;
						}
					}
					if (!foundMatching)
					{
						continue;
					}
				}

				if (m_debugGui->TreeNode(GetEntityNameWithTags(entityID).c_str()))
				{
					std::vector<ComponentType> owned = m_world->GetOwnedComponentTypes(entityID);
					for (const auto& cmp : owned)
					{
						if (m_debugGui->TreeNode(cmp.c_str()))
						{
							auto inspector = m_componentInspectors.find(cmp);
							if (inspector != m_componentInspectors.end())
							{
								inspector->second(*m_world->GetStorage(cmp), entityID);
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

bool EntitySystem::PreInit()
{
	SDE_PROF_EVENT();

	m_scriptSystem = Engine::GetSystem<Engine::ScriptSystem>("Script");
	assert(m_scriptSystem);
	m_debugGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
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