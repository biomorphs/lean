#include "transform.h"

COMPONENT_BEGIN(Transform,
	"SetPosition", &Transform::SetPosition,
	"SetRotation", &Transform::SetRotation,
	"SetScale", &Transform::SetScale,
	"GetPosition", &Transform::GetPosition
)
COMPONENT_END()