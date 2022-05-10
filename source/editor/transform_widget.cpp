#include "transform_widget.h"
#include "utils.h"
#include "editor.h"
#include "engine/system_manager.h"
#include "engine/components/component_transform.h"
#include "entity/entity_system.h"
#include "engine/graphics_system.h"
#include "engine/input_system.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_render.h"
#include "engine/intersection_tests.h"
#include "commands/editor_set_entity_positions_cmd.h"

glm::vec3 TransformWidget::FindCenterPosition(const std::vector<EntityHandle>& entities)
{
	auto esys = Engine::GetSystem<EntitySystem>("Entities");
	auto world = esys->GetWorld();
	glm::vec3 newPosition(0.0f, 0.0f, 0.0f);
	for (auto handle : entities)
	{
		auto transformCmp = world->GetComponent<Transform>(handle);
		if (transformCmp)
		{
			newPosition = newPosition + glm::vec3(transformCmp->GetWorldspaceMatrix()[3]);
		}
	}
	if (entities.size() > 0)
	{
		newPosition = newPosition / static_cast<float>(entities.size());
	}
	return newPosition;
}

void TransformWidget::Reset(const std::vector<EntityHandle>& entities)
{
	SDE_PROF_EVENT();

	// first find the new position of the widget from the world-space positions of the entities
	auto esys = Engine::GetSystem<EntitySystem>("Entities");
	auto world = esys->GetWorld();
	glm::vec3 newPosition = FindCenterPosition(entities);
	m_widgetTransform = glm::translate(newPosition);

	// track the entities + world space transforms for later
	m_trackedEntities.clear();
	for (auto handle : entities)
	{
		auto transformCmp = world->GetComponent<Transform>(handle);
		if (transformCmp)
		{
			m_trackedEntities.push_back({ handle, glm::vec3(transformCmp->GetWorldspaceMatrix()[3]) });
		}
	}
	m_currentEntities = entities;
}

void TransformWidget::RestoreEntityTransforms()
{
	auto esys = Engine::GetSystem<EntitySystem>("Entities");
	auto world = esys->GetWorld();
	for (auto& it : m_trackedEntities)
	{
		auto transformCmp = world->GetComponent<Transform>(it.m_entity);
		if (transformCmp)
		{
			transformCmp->SetPosition(it.m_originalPosition);
		}
	}
}

void TransformWidget::CommitTranslation(glm::vec3 translation)
{
	auto editor = Engine::GetSystem<Editor>("Editor");
	auto cmd = std::make_unique<EditorSetEntityPositionsCommand>();
	for (auto it : m_trackedEntities)
	{
		cmd->AddEntity(it.m_entity, it.m_originalPosition, it.m_originalPosition + translation);
	}
	editor->PushCommand(std::move(cmd));
}

bool TransformWidget::HandleMouseOverTranslateHandle(Axis axis,
	glm::vec3 mouseRayStart, glm::vec3 mouseRayEnd, bool mouseDownThisFrame, 
	float barWidth, float widgetScale)
{
	auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	auto& dbg = graphics->DebugRenderer();
	glm::vec3 widgetPos = glm::vec3(m_widgetTransform[3]);

	// x
	glm::vec3 boxMin = { 0.0f, -barWidth / 2.0f, -barWidth / 2.0f };
	glm::vec3 boxMax = { widgetScale, barWidth / 2.0f, barWidth / 2.0f };
	glm::vec3 axisDirection = AxisVector(axis);
	glm::vec4 colour = { axisDirection, 1.0f };
	if (axis == Axis::Y)
	{
		boxMin = { -barWidth / 2.0f, 0.0f, -barWidth / 2.0f };
		boxMax = { barWidth / 2.0f, widgetScale, barWidth / 2.0f };
	}
	else if (axis == Axis::Z)
	{
		boxMin = { -barWidth / 2.0f, -barWidth / 2.0f, 0.0f };
		boxMax = { barWidth / 2.0f, barWidth / 2.0f, widgetScale };
	}

	float hitT = 0.0f;
	if (Engine::RayIntersectsAABB(mouseRayStart, mouseRayEnd, widgetPos + boxMin, widgetPos + boxMax, hitT))
	{
		colour = { 1.0f,1.0f,1.0f,1.0f };
		if (mouseDownThisFrame && !m_mouseBtnDownLastFrame)
		{
			glm::vec3 widgetAxis = glm::normalize(glm::mat3(m_widgetTransform) * axisDirection) * widgetScale;
			glm::vec3 c0, c1;	// closest points from mouse ray to axis (one point per ray)
			if (Engine::GetNearPointsBetweenLines(mouseRayStart, mouseRayEnd, widgetPos, widgetPos + widgetAxis, c0, c1))
			{
				m_mouseDragStartTransform = m_widgetTransform;
				m_mouseClickPosOnAxis = c1;
				m_dragAxis = axis;
				return true;
			}
		}
	}
	dbg.DrawBox(boxMin, boxMax, colour, m_widgetTransform);

	return false;
}

glm::vec3 TransformWidget::AxisVector(Axis axis)
{
	switch (axis)
	{
	case Axis::X:
		return { 1.0f,0.0f,0.0f };
	case Axis::Y:
		return { 0.0f,1.0f,0.0f };
	case Axis::Z:
		return { 0.0f,0.0f,1.0f };
	default:
		return { 0.0f,0.0f,0.0f };
	}
}

void TransformWidget::Update(const std::vector<EntityHandle>& entities)
{
	SDE_PROF_EVENT();

	float widgetScale = 128.0f;	// todo - rescale based on screen space size
	float barWidth = 2.0f;	// rescale from ss size

	auto graphics = Engine::GetSystem<GraphicsSystem>("Graphics");
	auto input = Engine::GetSystem<Engine::InputSystem>("Input");
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto esys = Engine::GetSystem<EntitySystem>("Entities");
	auto world = esys->GetWorld();
	auto& dbg = graphics->DebugRenderer();	// todo - need a debug render option with no depth read

	if (entities != m_currentEntities)
	{
		Reset(entities);
	}

	if (m_trackedEntities.size() > 0)
	{
		bool leftBtnDown = false;
		if (!gui->IsCapturingMouse() && !gui->IsCapturingKeyboard())
		{
			leftBtnDown = input->GetMouseState().m_buttonState & Engine::MouseButtons::LeftButton;
		}

		glm::vec3 widgetPos = glm::vec3(m_widgetTransform[3]);
		glm::vec3 mouseRayStartWs, mouseRayEndWs;
		EditorUtils::MouseCursorToWorldspaceRay(1000.0f, mouseRayStartWs, mouseRayEndWs);
		glm::mat4 widgetInverse = glm::inverse(m_widgetTransform);

		// dragging...
		if (m_mouseBtnDownLastFrame && m_dragAxis != Axis::None)
		{
			glm::vec3 originalPos = glm::vec3(m_mouseDragStartTransform[3]);
			glm::vec3 widgetAxis = glm::normalize(glm::mat3(m_mouseDragStartTransform) * AxisVector(m_dragAxis)) * widgetScale;
			glm::vec3 c0, c1;	// closest points from mouse ray to axis (one point per ray)
			if (Engine::GetNearPointsBetweenLines(mouseRayStartWs, mouseRayEndWs, originalPos, originalPos + widgetAxis, c0, c1))
			{
				dbg.DrawLine(c0, c1, { 1.0f,1.0f,0.0f,1.0f });
				dbg.AddAxisAtPoint(glm::vec4(c1, 1.0f), 8.0f);
				glm::vec3 translation = c1 - originalPos;
				if (leftBtnDown)
				{
					m_widgetTransform = glm::translate(originalPos);
					for (auto it : m_trackedEntities)
					{
						auto transformCmp = world->GetComponent<Transform>(it.m_entity);
						if (transformCmp)
						{
							glm::vec3 newPos = it.m_originalPosition + translation;
							transformCmp->SetPosition(newPos);	// todo - needs to be set worldspace position
						}
					}
				}
				else    // just let go of mouse
				{
					CommitTranslation(translation);
					Reset(entities);
					m_dragAxis = Axis::None;
				}
			}
		}

		if (HandleMouseOverTranslateHandle(Axis::X, mouseRayStartWs, mouseRayEndWs, leftBtnDown, barWidth, widgetScale))
		{
		}
		else if (HandleMouseOverTranslateHandle(Axis::Y, mouseRayStartWs, mouseRayEndWs, leftBtnDown, barWidth, widgetScale))
		{
		}
		else if(HandleMouseOverTranslateHandle(Axis::Z, mouseRayStartWs, mouseRayEndWs, leftBtnDown, barWidth, widgetScale))
		{
		}

		m_mouseBtnDownLastFrame = leftBtnDown;
	}
}