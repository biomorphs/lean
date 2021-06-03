#include "component_camera.h"

COMPONENT_SCRIPTS(Camera,
	"SetNearPlane", &Camera::SetNearPlane,
	"SetFarPlane", &Camera::SetFarPlane,
	"SetFOV", &Camera::SetFOV
)
