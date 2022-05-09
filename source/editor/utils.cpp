#include "utils.h"
#include "engine/system_manager.h"
#include "engine/input_system.h"
#include "engine/camera_system.h"
#include "engine/render_system.h"
#include "render/camera.h"
#include "render/window.h"

namespace EditorUtils
{
	void MouseCursorToWorldspaceRay(float rayDistance, glm::vec3& rayStart, glm::vec3& rayEnd)
	{
		auto input = Engine::GetSystem<Engine::InputSystem>("Input");
		const auto& mainCam = Engine::GetSystem<Engine::CameraSystem>("Cameras")->MainCamera();
		const auto windowSize = glm::vec2(Engine::GetSystem<Engine::RenderSystem>("Render")->GetWindow()->GetSize());
		const glm::vec2 cursorPos(input->GetMouseState().m_cursorX, windowSize.y - input->GetMouseState().m_cursorY);
		glm::vec3 mouseWorldSpace = mainCam.WindowPositionToWorldSpace(cursorPos, windowSize);
		const glm::vec3 lookDirWorldspace = glm::normalize(mouseWorldSpace - mainCam.Position());
		rayStart = mainCam.Position();
		rayEnd = mainCam.Position() + lookDirWorldspace * rayDistance;
	}
}