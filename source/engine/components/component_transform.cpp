#include "component_transform.h"

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