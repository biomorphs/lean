#include "transform_widget.h"
#include "engine/system_manager.h"
#include "engine/components/component_transform.h"
#include "entity/entity_system.h"
#include "engine/graphics_system.h"
#include "engine/debug_render.h"

void TransformWidget::Reset(const std::vector<EntityHandle>& entities)
{
	// first find the new position of the widget from the world-space positions of the entities
	auto esys = Engine::GetSystem<EntitySystem>("Entities");
	auto world = esys->GetWorld();
	glm::vec3 newPosition(0.0f, 0.0f, 0.0f);
	for (auto handle : entities)
	{
		auto transformCmp = world->GetComponent<Transform>(handle);
		if (transformCmp)
		{
			newPosition = newPosition + transformCmp->GetPosition();
		}
	}
	if (entities.size() > 0)
	{
		newPosition = newPosition / static_cast<float>(entities.size());
	}

	m_widgetTransform = glm::translate(newPosition);
	glm::mat4 widgetInverse = glm::inverse(m_widgetTransform);

	m_trackedEntities.clear();

	// now calculate transforms for each entity relative to the widget
	for (auto it : entities)
	{
		auto transformCmp = world->GetComponent<Transform>(it);
		if (transformCmp)
		{
			m_trackedEntities.push_back({it, transformCmp->GetWorldspaceMatrix() * widgetInverse});
		}
	}
	m_currentEntities = entities;
}

void TransformWidget::Update(const std::vector<EntityHandle>& entities)
{
	if (entities != m_currentEntities)
	{
		Reset(entities);
	}

	if (m_trackedEntities.size() > 0)
	{
		auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");

		glm::vec3 widgetPos = glm::vec3(m_widgetTransform[3]);
		float widgetScale = 8.0f;	// todo - rescale based on screen space size
		float barWidth = 0.25f;	// rescale from ss size

		// todo - need a debug render option with no depth read
		auto& dbg = graphics->DebugRenderer();

		// x
		dbg.DrawBox({ 0.0f, -barWidth / 2.0f, -barWidth / 2.0f }, { widgetScale, barWidth / 2.0f, barWidth / 2.0f }, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), m_widgetTransform);

		// y
		dbg.DrawBox({ -barWidth / 2.0f, 0.0f, -barWidth / 2.0f }, { barWidth / 2.0f, widgetScale, barWidth / 2.0f }, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), m_widgetTransform);
		
		// z
		dbg.DrawBox({ -barWidth / 2.0f, -barWidth / 2.0f, 0.0f }, { barWidth / 2.0f, barWidth / 2.0f, widgetScale }, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), m_widgetTransform);

		dbg.AddAxisAtPoint(glm::vec4(widgetPos,0.0f), widgetScale);
	}
}