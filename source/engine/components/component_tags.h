#pragma once

#include "entity/component.h"
#include "engine/tag.h"
#include <set>
#include <unordered_set>
#include <robin_hood.h>

class Tags
{
public:
	COMPONENT(Tags);

	void AddTag(const Engine::Tag& t);
	void RemoveTag(const Engine::Tag& t);
	bool ContainsTag(const Engine::Tag& t) const;

	using TagSet = std::vector<Engine::Tag>;
	const TagSet& AllTags() const { return m_tags; }
private:
	TagSet m_tags;
};