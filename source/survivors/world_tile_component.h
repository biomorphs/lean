#pragma once

#include "entity/component.h"

class WorldTileComponent
{
public:
	COMPONENT(WorldTileComponent);
	COMPONENT_INSPECTOR();

	std::vector<EntityHandle>& OwnedChildren() { return m_ownedChildren; }

private:
	std::vector<EntityHandle> m_ownedChildren;
};