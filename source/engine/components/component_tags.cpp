#include "component_tags.h"

COMPONENT_SCRIPTS(Tags,
	"AddTag", &Tags::AddTag,
	"RemoveTag", &Tags::RemoveTag,
	"ContainsTag", &Tags::ContainsTag,
	"AllTags", &Tags::AllTags
);

void Tags::AddTag(Engine::Tag t)
{
	m_tags.insert(t);
}

void Tags::RemoveTag(Engine::Tag t)
{
	m_tags.erase(t);
}

bool Tags::ContainsTag(Engine::Tag t)
{
	return m_tags.find(t) != m_tags.end();
}