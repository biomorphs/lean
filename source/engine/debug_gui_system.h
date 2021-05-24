#pragma once

#include "system.h"
#include "core/glm_headers.h"
#include <IconsForkAwesome.h>	// expose fork-awesome icons to all users
#include <memory>
#include <vector>
#include <string>

namespace Render
{
	class Texture;
}

namespace Engine
{
	class RenderSystem;
	class ScriptSystem;
	class ImguiSdlGL3RenderPass;
	class GraphDataBuffer;
	class MenuBar;
	class DebugGuiSystem : public System
	{
	public:
		DebugGuiSystem();
		virtual ~DebugGuiSystem();

		virtual bool PreInit(SystemManager& manager);
		virtual bool Initialise() override;
		virtual bool PostInit() override;
		virtual bool Tick(float timeDelta) override;
		virtual void Shutdown() override;
		bool IsCapturingMouse();	// if true the gui is interacting with mouse
		bool IsCapturingKeyboard();	// if true the gui is interacting with keyboard
		bool BeginWindow(bool& windowOpen, const char* windowName, glm::vec2 size=glm::vec2(-1.f));
		void EndWindow();
		void ItemWidth(float w);
		void SameLine(float xOffset = 0.0f, float spacing = -1.0f);
		void Text(const char* txt);
		bool TextInput(const char* label, char* textBuffer, size_t bufferSize);
		bool TextInput(const char* label, std::string& str);
		bool Button(const char* txt);
		bool Selectable(const char* txt, bool selected = false);
		void Separator();
		bool ComboBox(const char* label, const char* items[], int itemCount, int &currentItem);
		bool ComboBox(const char* label, std::vector<std::string> items, int& currentItem);
		bool Checkbox(const char* text, bool val);
		glm::vec4 ColourEdit(const char* label, glm::vec4 c, bool showAlpha = true);
		float DragFloat(const char* label, float f, float step = 1.0f, float min = 0.0f, float max = 0.0f);
		int32_t DragInt(const char* label, int32_t t, int32_t step = 1, int32_t min = 0, int32_t max = 0);
		glm::vec4 DragVector(const char* label, glm::vec4 v, float step = 1.0f, float min = 0.0f, float max = 0.0f);
		glm::vec3 DragVector(const char* label, glm::vec3 v, float step = 1.0f, float min = 0.0f, float max = 0.0f);
		void Image(Render::Texture& src, glm::vec2 size, glm::vec2 uv0 = glm::vec2(0.0f,0.0f), glm::vec2 uv1 = glm::vec2(1.0f,1.0f));
		void GraphLines(const char* label, glm::vec2 size, const std::vector<float>& values);
		void GraphLines(const char* label, glm::vec2 size, GraphDataBuffer& buffer);
		void GraphHistogram(const char* label, glm::vec2 size, GraphDataBuffer& buffer);
		void MainMenuBar(MenuBar&);
		bool TreeNode(const char* label, bool forceExpanded = false);
		void TreePop();

	private:
		RenderSystem* m_renderSystem;
		std::unique_ptr<ImguiSdlGL3RenderPass> m_imguiPass;
		Engine::ScriptSystem* m_scriptSystem = nullptr;
	};
}