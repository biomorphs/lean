#include "walkable_area.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"
#include "engine/debug_gui_system.h"
#include "core/random.h"

COMPONENT_SCRIPTS(WalkableArea,
	"SetBounds", &WalkableArea::SetBounds,
	"SetGridResolution", &WalkableArea::SetGridResolution
)

SERIALISE_BEGIN(WalkableArea)
SERIALISE_PROPERTY("BoundsMin", m_boundsMin)
SERIALISE_PROPERTY("BoundsMax", m_boundsMax)
SERIALISE_PROPERTY("GridResolution", m_gridResolution)
SERIALISE_PROPERTY("Grid", m_grid)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(WalkableArea, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& a = *static_cast<StorageType&>(cs).Find(e);
		i.Inspect("Min. Bounds", a.GetBoundsMin(), InspectFn(e, &WalkableArea::SetBoundsMin));
		i.Inspect("Max. Bounds", a.GetBoundsMax(), InspectFn(e, &WalkableArea::SetBoundsMax));

		auto gridRes = a.GetGridResolution();
		gridRes.x = gui.DragInt("Grid Res. X", gridRes.x, 1);
		gridRes.y = gui.DragInt("Grid Res. Y", gridRes.y, 1);
		if (gridRes != a.GetGridResolution())
		{
			a.SetGridResolution(gridRes);
		}
		if (gui.Button("Random!"))
		{
			for (int x = 0; x < gridRes.x; ++x)
			{
				for (int z = 0; z < gridRes.y; ++z)
				{
					if (Core::Random::GetInt(0, 100) < 30)
					{
						a.SetGridValue({ x,z }, -1.0f);
					}
					else if (Core::Random::GetInt(0, 100) < 50)
					{
						a.SetGridValue({ x,z }, Core::Random::GetFloat(a.GetBoundsMin().y, a.GetBoundsMax().y * 0.2f));
					}
				}
			}
		}
	};
	return fn;
}

void WalkableArea::SetGridValue(glm::ivec2 pos, float value)
{
	if (glm::all(glm::greaterThanEqual(pos, { 0,0 })) && glm::all(glm::lessThan(pos, m_gridResolution)))
	{
		m_grid[pos.x + pos.y * m_gridResolution.x] = value;
	}
}

float WalkableArea::GetGridValue(glm::ivec2 pos)
{
	if (glm::all(glm::greaterThanEqual(pos, { 0,0 })) && glm::all(glm::lessThan(pos, m_gridResolution)))
	{
		return m_grid[pos.x + pos.y * m_gridResolution.x];
	}
	else
	{
		return 0.0f;
	}
}

void WalkableArea::SetBounds(glm::vec3 bMin, glm::vec3 bMax)
{
	m_boundsMin = bMin;
	m_boundsMax = bMax;
}

void WalkableArea::SetGridResolution(glm::ivec2 r)
{
	if (m_gridResolution != r)
	{
		m_grid.resize(r.x * (size_t)r.y);
		std::fill(m_grid.begin(), m_grid.end(), m_boundsMin.y);
		m_gridResolution = r;
	}
}