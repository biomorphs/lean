#include "walkable_system.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/render_system.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"
#include "engine/renderer.h"
#include "core/profiler.h"
#include "entity/entity_system.h"
#include "engine/components/component_transform.h"
#include "walkable_area.h"

WalkableSystem::WalkableSystem()
{
}

WalkableSystem::~WalkableSystem()
{
}

void WalkableSystem::RegisterComponents()
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	entities->RegisterComponentType<WalkableArea>();
	entities->RegisterInspector<WalkableArea>(WalkableArea::MakeInspector(*dbgGui));
}

bool WalkableSystem::PreInit()
{
	SDE_PROF_EVENT();

	return true;
}

bool WalkableSystem::Initialise()
{
	SDE_PROF_EVENT();

	RegisterComponents();

	return true;
}

bool WalkableSystem::Tick(float timeDelta)
{
	SDE_PROF_EVENT();

	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	auto& dbgRender = graphics->DebugRenderer();

	static auto walkables = entities->GetWorld()->MakeIterator<WalkableArea, Transform>();
	walkables.ForEach([&dbgRender](WalkableArea& area, Transform& t, EntityHandle e) {
		const glm::vec4 colour(0.0f, 0.2f, 0.2f, 1.0f);
		dbgRender.DrawBox(area.GetBoundsMin(), area.GetBoundsMax(), colour, t.GetWorldspaceMatrix());

		auto gridRes = area.GetGridResolution();
		if (glm::all(glm::greaterThan(gridRes, { 1,1 })))
		{
			glm::vec3 boundsSize = area.GetBoundsMax() - area.GetBoundsMin();
			glm::vec2 cellSize = { boundsSize.x / (gridRes.x-1), boundsSize.z / (gridRes.y-1) };
			glm::mat4 wsMatrix = t.GetWorldspaceMatrix();
			for (int x = 0; x < gridRes.x-1; ++x)
			{
				for (int z = 0; z < gridRes.y-1; ++z)
				{
					float value = area.GetGridValue({ x, z });
					if (value >= 0.0f)
					{
						glm::vec4 bl(area.GetBoundsMin().x + cellSize.x * (float)x, value, area.GetBoundsMin().z + cellSize.y * (float)z, 1.0f);
						glm::vec4 tr = bl + glm::vec4(cellSize.x, 0.0f, cellSize.y, 0.0f);
						float c0 = value;
						float c1 = area.GetGridValue({ x,z + 1 });
						float c2 = area.GetGridValue({ x + 1,z });
						float c3 = area.GetGridValue({ x + 1, z + 1 });
						glm::vec4 lines[] = {
							bl, {tr.x, c2, bl.z,1.0f},
							{bl.x,c1,tr.z,1.0f}, {tr.x, c3, tr.z,1.0f},
							bl, {bl.x, c1, tr.z, 1.0f},
							{tr.x,c2,bl.z,1.0f}, {tr.x, c3, tr.z, 1.0f}
						};
						glm::vec4 colours[] = {
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
							{1.0f,1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f,1.0f},
						};
						for (auto& l : lines)
						{
							l = wsMatrix * l;
						}
						dbgRender.AddLines(lines, colours, 4);
					}
				}
			}
		}
	});

	return true;
}

void WalkableSystem::Shutdown()
{
	SDE_PROF_EVENT();
}