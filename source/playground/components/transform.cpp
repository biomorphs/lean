#include "transform.h"

COMPONENT_SCRIPTS(Transform,
	"SetPosition", &Transform::SetPos3,
	"SetRotation", &Transform::SetRotation,
	"SetScale", &Transform::SetScale3,
	"GetPosition", &Transform::GetPosition,
	"GetRotationDegrees", &Transform::GetRotationDegrees
)