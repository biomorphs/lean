#include "component_transform.h"
#include "engine/debug_render.h"
#include "entity/entity_handle.h"
#include "entity/component_inspector.h"

COMPONENT_SCRIPTS(Transform,
	"SetPosition", &Transform::SetPos3,
	"SetRotation", &Transform::SetRotationDegrees3,
	"SetScale", &Transform::SetScale3,
	"GetScale", &Transform::GetScale,
	"GetPosition", &Transform::GetPosition,
	"GetRotationDegrees", &Transform::GetRotationDegrees,
	"RotateEuler", &Transform::RotateEuler
)

SERIALISE_BEGIN(Transform)
	SERIALISE_PROPERTY("Position", m_position)
	SERIALISE_PROPERTY("Orientation", m_orientation)
	SERIALISE_PROPERTY("Scale", m_scale)
	SERIALISE_PROPERTY("Parent", m_parent)
	if (op == Engine::SerialiseType::Read)
	{
		RebuildMatrix();
	}
SERIALISE_END()

void Transform::SetParent(EntityHandle parent)
{
	m_parent = parent;
}

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

glm::mat4 Transform::GetWorldspaceMatrix()
{
	auto parent = m_parent.GetComponent();
	if (parent)
	{
		return parent->GetWorldspaceMatrix() * m_matrix;
	}
	else
	{
		return m_matrix;
	}
}

bool HasParent(Transform* parent, Transform* child)
{
	Transform* thisParent = child->GetParent().GetComponent();
	if (thisParent == parent || thisParent == child)
	{
		return true;
	}
	else if(thisParent)
	{
		return HasParent(parent, thisParent);
	}
	else
	{
		return false;
	}
}

COMPONENT_INSPECTOR_IMPL(Transform, Engine::DebugGuiSystem& gui, Engine::DebugRender& render)
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	auto fn = [&gui, &render, entities](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		static EntityHandle setParentEntity;
		auto& t = *static_cast<Transform::StorageType&>(cs).Find(e);
		i.Inspect("Position", t.GetPosition(), InspectFn(e, &Transform::SetPosition), 0.25f, -1000000.0f, 1000000.0f);
		i.Inspect("Rotation", t.GetRotationRadians(), InspectFn(e, &Transform::SetRotationRadians), 0.1f);
		i.Inspect("Scale", t.GetScale(), InspectFn(e, &Transform::SetScale), 0.1f, 0.0f);
		i.Inspect("Parent", t.GetParent().GetEntity(), InspectFn(e, &Transform::SetParent), [&e, &t, entities](const EntityHandle& p) {
			auto tp = entities->GetWorld()->GetComponent<Transform>(p);
			if (tp)
			{
				if (!HasParent(&t, tp))	// protect against loops
				{
					return true;
				}
			}
			return false;
		});

		glm::vec4 p0 = glm::vec4(t.GetPosition(), 1.0f);
		glm::mat4 mat = t.GetWorldspaceMatrix();
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