#include "component_transform.h"

COMPONENT_SCRIPTS(Transform,
	"SetPosition", &Transform::SetPos3,
	"SetRotation", &Transform::SetRotation,
	"SetScale", &Transform::SetScale3,
	"GetPosition", &Transform::GetPosition,
	"GetRotationDegrees", &Transform::GetRotationDegrees
)

void Transform::RebuildMatrix()
{
	glm::mat4 modelMat = glm::translate(glm::identity<glm::mat4>(), m_position);
	modelMat = glm::rotate(modelMat, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	modelMat = glm::rotate(modelMat, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::rotate(modelMat, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
	m_matrix = glm::scale(modelMat, m_scale);
}