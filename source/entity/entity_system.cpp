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

nlohmann::json EntitySystem::SerialiseEntities(const std::vector<uint32_t>& entityIDs)
{
	nlohmann::json json;

	const uint32_t entityCount = entityIDs.size();
	Engine::ToJson("EntityCount", entityCount, json);
	Engine::ToJson("EntityIDs", entityIDs, json);

	std::vector<nlohmann::json> allEntityData;
	allEntityData.reserve(entityCount);
	for (int index = 0; index < entityCount; ++index)
	{
		auto& entityJson = allEntityData.emplace_back();
		Engine::ToJson("ID", entityIDs[index], entityJson);

		std::vector<ComponentType> cmpTypes = m_world->GetOwnedComponentTypes(entityIDs[index]);
		std::vector<nlohmann::json> componentData;
		componentData.reserve(cmpTypes.size());
		for (const auto& type : cmpTypes)
		{
			nlohmann::json& cmpJson = componentData.emplace_back();
			cmpJson["Type"] = type;
			ComponentStorage* storage = m_world->GetStorage(type);
			nlohmann::json& dataJson = cmpJson["Data"];
			storage->Serialise(entityIDs[index], dataJson, Engine::SerialiseType::Write);
		}
		entityJson["Components"] = componentData;
	}
	json["Entities"] = allEntityData;

	return json;
}

std::vector<uint32_t> EntitySystem::SerialiseEntities(nlohmann::json& data, bool restoreIDsFromData)
{
	uint32_t entityCount = 0;
	Engine::FromJson("EntityCount", entityCount, data);

	std::vector<uint32_t> oldEntityIDs;
	Engine::FromJson("EntityIDs", oldEntityIDs, data);
	if (oldEntityIDs.size() != entityCount)
	{
		SDE_LOG("Error - ID list size does not match entity count! Bad data?");
		return {};
	}

	// First create the entities and build a remap table for the IDs
	std::unordered_map<uint32_t, uint32_t> oldEntityToNewEntity;
	{
		SDE_PROF_EVENT("AddEntities");
		oldEntityToNewEntity.reserve(oldEntityIDs.size());
		if (restoreIDsFromData)
		{
			for (uint32_t oldID : oldEntityIDs)
			{
				auto newEntity = m_world->AddEntityFromHandle(oldID);
				if (newEntity.GetID() == -1)
				{
					SDE_LOG("Error - failed to recreate entity with fixed ID %d. References to this entity will break!", oldID);
					newEntity = m_world->AddEntity().GetID();
				}
				oldEntityToNewEntity[oldID] = newEntity.GetID();
			}
		}
		else
		{
			for (uint32_t oldID : oldEntityIDs)
			{
				oldEntityToNewEntity[oldID] = m_world->AddEntity().GetID();
			}
		}
	}

	// Hook the OnLoaded callback of EntityHandle and remap any IDs we find during loading
	// This is how references to entities are fixed up!
	EntityHandle::OnLoaded remapId = [&oldEntityToNewEntity](EntityHandle& h) {
		if (h.GetID() != -1)
		{
			auto foundRemap = oldEntityToNewEntity.find(h.GetID());
			if (foundRemap != oldEntityToNewEntity.end())
			{
				h = EntityHandle(foundRemap->second);
			}
			else
			{
				SDE_LOG("Entity handle references ID that doesn't exist in the scene!");
			}
		}
	};
	EntityHandle::SetOnLoadFinishCallback(remapId);

	// Load the data!
	for (int index = 0; index < entityCount; ++index)
	{
		nlohmann::json& thisEntityData = data["Entities"][index];
		uint32_t oldId = 0;
		Engine::FromJson("ID", oldId, thisEntityData);		// this is the OLD id
		const uint32_t remappedId = oldEntityToNewEntity[oldId];
		auto& cmpList = thisEntityData["Components"];
		int componentCount = cmpList.size();
		for (int component = 0; component < componentCount; ++component)
		{
			std::string typeStr;
			nlohmann::json& cmpJson = cmpList[component];
			Engine::FromJson("Type", typeStr, cmpJson);

			ComponentStorage* storage = m_world->GetStorage(typeStr);
			if (storage)
			{
				nlohmann::json& dataJson = cmpJson["Data"];
				storage->Serialise(remappedId, dataJson, Engine::SerialiseType::Read);
			}
		}
	}

	EntityHandle::SetOnLoadFinishCallback(nullptr);

	std::vector<uint32_t> results;
	results.reserve(oldEntityToNewEntity.size());
	for (const auto& it : oldEntityToNewEntity)
	{
		results.emplace_back(it.second);
	}
	return results;
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

void EntitySystem::ShowInspector(const std::vector<uint32_t>& entities, bool expandAll, const char* titleText)
{
	SDE_PROF_EVENT();
	static std::string filterText = "";

	bool keepOpen = true;
	m_debugGui->BeginWindow(keepOpen, titleText);
	m_debugGui->TextInput("Tag Filter", filterText);
	if (m_debugGui->TreeNode("All Entities", true))
	{
		auto allComponentTypes = m_world->GetAllComponentTypes();
		for (auto entityID : entities)
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
			if (m_debugGui->TreeNode(GetEntityNameWithTags(entityID).c_str(), expandAll))
			{
				std::vector<ComponentType> owned = m_world->GetOwnedComponentTypes(entityID);
				for (const auto& cmp : owned)
				{
					if (m_debugGui->TreeNode(cmp.c_str(), expandAll))
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

bool EntitySystem::PreInit()
{
	SDE_PROF_EVENT();

	m_scriptSystem = Engine::GetSystem<Engine::ScriptSystem>("Script");
	assert(m_scriptSystem);
	m_debugGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	assert(m_debugGui);

	auto& scripts = m_scriptSystem->Globals();

	// Tag
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
	menu.AddItem("List", []() {	g_showWindow = !g_showWindow; });

	return true;
}

bool EntitySystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	if (g_showWindow)
	{
		ShowInspector(m_world->AllEntities());
	}
	m_debugGui->MainMenuBar(g_entityMenu);

	m_world->CollectGarbage();
	return true;
}

void EntitySystem::Shutdown()
{
	SDE_PROF_EVENT();
	m_world = nullptr;
}