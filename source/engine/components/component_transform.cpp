#include "component_transform.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_render.h"
#include "entity/entity_handle.h"

COMPONENT_SCRIPTS(Transform,
	"SetPosition", &Transform::SetPos3,
	"SetRotation", &Transform::SetRotationDegrees3,
	"SetScale", &Transform::SetScale3,
	"GetScale", &Transform::GetScale,
	"GetPosition", &Transform::GetPosition,
	"GetRotationDegrees", &Transform::GetRotationDegrees,
	"RotateEuler", &Transform::RotateEuler
)

void Transform::RebuildMatrix()
{
	glm::mat4 modelMat = glm::translate(glm::identity<glm::mat4>(), m_position);
	glm::mat4 rotation = glm::toMat4(m_orientation);
	modelMat = modelMat * rotation;
	m_matrix = glm::scale(modelMat, m_scale);
}

void Transform::RotateEuler(glm::vec3 anglesRadians)
{
	glm::quat q(anglesRadians);
	m_orientation = m_orientation * q;
	RebuildMatrix();
}

COMPONENT_INSPECTOR_IMPL(Transform, Engine::DebugGuiSystem& gui, Engine::DebugRender& render)
{
	auto fn = [&gui, &render](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& t = *static_cast<Transform::StorageType&>(cs).Find(e);
		t.SetPosition(gui.DragVector("Position", t.GetPosition(), 0.25f, -100000.0f, 100000.0f));
		t.SetRotationDegrees(gui.DragVector("Rotation", t.GetRotationDegrees(), 0.1f));
		t.SetScale(gui.DragVector("Scale", t.GetScale(), 0.05f, 0.0f));
		glm::vec4 p0 = glm::vec4(t.GetPosition(), 1.0f);
		glm::mat4 mat = t.GetMatrix();
		glm::vec4 lines[] = {
			p0, mat * glm::vec4(1.0f,0.0f,0.0f,1.0f),
			p0, mat * glm::vec4(0.0f,1.0f,0.0f,1.0f),
			p0, mat * glm::vec4(0.0f,0.0f,1.0f,1.0f),
		};
		glm::vec4 colours[] = {
			{1.0f,0.0f,0.0f,1.0f},{1.0f,0.0f,0.0f,1.0f},
			{0.0f,1.0f,0.0f,1.0f},{0.0f,1.0f,0.0f,1.0f},
			{0.0f,0.0f,1.0f,1.0f},{0.0f,0.0f,1.0f,1.0f},
		};
		render.AddLines(lines, colours, 3);
	};
	return fn;
}