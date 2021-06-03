#include "component_camera.h"
#include "engine/debug_gui_system.h"
#include "entity/entity_handle.h"

COMPONENT_SCRIPTS(Camera,
	"SetNearPlane", &Camera::SetNearPlane,
	"SetFarPlane", &Camera::SetFarPlane,
	"SetFOV", &Camera::SetFOV
)

COMPONENT_INSPECTOR_IMPL(Camera, Engine::DebugGuiSystem& gui)
{
	auto fn = [&gui](ComponentStorage& cs, const EntityHandle& e)
	{
		auto& c = *static_cast<StorageType&>(cs).Find(e);
		c.SetNearPlane(gui.DragFloat("Near Plane", c.GetNearPlane(), 0.01f, 0.0f, c.GetFarPlane()));
		c.SetFarPlane(gui.DragFloat("Far Plane", c.GetFarPlane(), 0.01f, c.GetNearPlane(), 100000000.0f));
		c.SetFOV(gui.DragFloat("FOV", c.GetFOV(), 0.1f, 0.0f, 180.0f));
	};
	return fn;
}