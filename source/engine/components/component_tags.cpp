#include "component_tags.h"

COMPONENT_SCRIPTS(Tags,
	"AddTag", &Tags::AddTag,
	"RemoveTag", &Tags::RemoveTag,
	"ContainsTag", &Tags::ContainsTag
);

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