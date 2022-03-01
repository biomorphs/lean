#include "component_tags.h"
#include "engine/debug_gui_system.h"
#include "entity/entity_handle.h"

COMPONENT_SCRIPTS(Tags,
	"AddTag", &Tags::AddTag,
	"RemoveTag", &Tags::RemoveTag,
	"ContainsTag", &Tags::ContainsTag
);

SERIALISE_BEGIN(Tags)
	SERIALISE_PROPERTY("Tags", m_tags)
SERIALISE_END()

void Tags::AddTag(const Engine::Tag& t)
{
	m_tags.push_back(t);
}

void Tags::RemoveTag(const Engine::Tag& t)
{
	auto found = std::find(m_tags.begin(), m_tags.end(), t);
	if (found != m_tags.end())
	{
		m_tags.erase(found);
	}
}

bool Tags::ContainsTag(const Engine::Tag& t) const
{
	return std::find(m_tags.begin(), m_tags.end(), t) != m_tags.end();
}

COMPONENT_INSPECTOR_IMPL(Tags, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& t = *static_cast<Tags::StorageType&>(cs).Find(e);
		for (const auto it : t.AllTags())
		{
			gui.Text(it.c_str());
		}
		static std::string newTagStr;
		gui.TextInput("New Tag", newTagStr);
		gui.SameLine();
		if (gui.Button("Add") && newTagStr.size() > 0)
		{
			t.AddTag(newTagStr.c_str());
			newTagStr = "";
		}
	};
	return fn;
}