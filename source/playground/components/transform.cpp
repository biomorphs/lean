#include "transform.h"

COMPONENT_BEGIN(Transform,
	"SetPosition", &Transform::SetPosition,
	"SetScale", &Transform::SetScale,
	"GetPosition", &Transform::GetPosition
)
COMPONENT_END()