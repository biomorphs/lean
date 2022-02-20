#include "component_transform.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_render.h"
#include "engine/system_manager.h"
#include "entity/entity_handle.h"
#include "entity/entity_system.h"

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
	auto fn = [&gui, &render, entities](ComponentStorage& cs, const EntityHandle& e)
	{
		static EntityHandle setParentEntity;
		auto& t = *static_cast<Transform::StorageType&>(cs).Find(e);
		t.SetPosition(gui.DragVector("Position", t.GetPosition(), 0.25f, -100000.0f, 100000.0f));
		t.SetRotationDegrees(gui.DragVector("Rotation", t.GetRotationDegrees(), 0.1f));
		t.SetScale(gui.DragVector("Scale", t.GetScale(), 0.05f, 0.0f));

		std::string parentName = t.GetParent().GetEntity().GetID() != -1 ?
			entities->GetEntityNameWithTags(t.GetParent().GetEntity()) : "No Parent";
		if (gui.Button(parentName.c_str()))
		{
			setParentEntity = e;
		}
		if (setParentEntity == e)
		{
			if (gui.BeginModalPopup("Select Parent"))
			{
				if (gui.Button("No Parent"))
				{
					t.SetParent({});
					setParentEntity = -1;
				}
				entities->GetWorld()->ForEachComponent<Transform>([entities, &gui, &t, &e](Transform& selParent, EntityHandle ent) {
					if (e.GetID() != ent.GetID() && !HasParent(&t, &selParent))
					{
						std::string entityName = entities->GetEntityNameWithTags(ent);
						if (gui.Button(entityName.c_str()))
						{
							t.SetParent({ ent });
							setParentEntity = -1;
						}
					}
				});
				gui.EndModalPopup();
			}
		}

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