#pragma once

#include "entity/component.h"
#include "engine/tag.h"
#include <set>

class Tags
{
public:
	COMPONENT(Tags);

	void AddTag(Engine::Tag t);
	void RemoveTag(Engine::Tag t);
	bool ContainsTag(Engine::Tag t);

	using TagSet = std::set<Engine::Tag>;
	const TagSet& AllTags() const { return m_tags; }
private:
	TagSet m_tags;
};