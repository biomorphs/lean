#include "transform.h"

COMPONENT_SCRIPTS(Transform,
	"SetPosition", &Transform::SetPosition,
	"SetRotation", &Transform::SetRotation,
	"SetScale", &Transform::SetScale,
	"GetPosition", &Transform::GetPosition,
	"GetRotationDegrees", &Transform::GetRotationDegrees
)