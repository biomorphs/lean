#include "component_camera.h"
#include "entity/component_inspector.h"

COMPONENT_SCRIPTS(Camera,
	"SetNearPlane", &Camera::SetNearPlane,
	"SetFarPlane", &Camera::SetFarPlane,
	"SetFOV", &Camera::SetFOV
)

SERIALISE_BEGIN(Camera)
	SERIALISE_PROPERTY("NearPlane", m_nearPlane)
	SERIALISE_PROPERTY("FarPlane", m_farPlane)
	SERIALISE_PROPERTY("FOV", m_fov)
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(Camera)
{
	auto fn = [](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto c = static_cast<StorageType&>(cs).Find(e);
		i.Inspect("Near Plane", c->GetNearPlane(), InspectFn(e, &Camera::SetNearPlane), 0.1f, 0.0f, c->GetFarPlane());
		i.Inspect("Far Plane", c->GetFarPlane(), InspectFn(e, &Camera::SetFarPlane), 0.1f, c->GetNearPlane(), 100000000.0f);
		i.Inspect("FOV", c->GetFOV(), InspectFn(e, &Camera::SetFOV), 0.1f, 0.0f, 180.0f);
	};
	return fn;
}