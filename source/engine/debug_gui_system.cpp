#include "debug_gui_system.h"
#include "imgui_sdl_gl3_render.h"
#include "graph_data_buffer.h"
#include "debug_gui_menubar.h"
#include "system_enumerator.h"
#include "render_system.h"
#include "event_system.h"
#include "render/texture.h"
#include "core/profiler.h"
#include <imgui\imgui.h>

namespace Engine
{
	DebugGuiSystem::DebugGuiSystem()
		: m_renderSystem(nullptr)
	{
	}

	DebugGuiSystem::~DebugGuiSystem()
	{
	}

	bool DebugGuiSystem::PreInit(SystemEnumerator& systemEnumerator)
	{
		SDE_PROF_EVENT();

		m_renderSystem = (RenderSystem*)systemEnumerator.GetSystem("Render");
		auto EventSystem = (Engine::EventSystem*)systemEnumerator.GetSystem("Events");
		EventSystem->RegisterEventHandler([this](void* e)
		{
			this->m_imguiPass->HandleEvent(e);
		});

		return true;
	}

	bool DebugGuiSystem::Initialise()
	{
		SDE_PROF_EVENT();
		return true;
	}

	bool DebugGuiSystem::PostInit()
	{
		SDE_PROF_EVENT();

		ImGui::CreateContext();
		m_imguiPass = std::make_unique<ImguiSdlGL3RenderPass>(m_renderSystem->GetWindow(), m_renderSystem->GetDevice());
		m_renderSystem->AddPass(*m_imguiPass, 0x10000000);	// high pass sort key so debug gui always renders last

		// merge awesome-fork icons with main font
		// allows use of icons in IconsForkAwesome.h
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();

		// merge in icons from Font Awesome
		static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
		ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
		io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FK, 16.0f, &icons_config, icons_ranges);

		return true;
	}

	void DoSubMenu(SubMenu& b)
	{
		if (ImGui::BeginMenu(b.m_label.c_str(), b.m_enabled))
		{
			for (auto item : b.m_menuItems)
			{
				if (ImGui::MenuItem(item.m_label.c_str(), item.m_shortcut.c_str(), item.m_selected, item.m_enabled))
				{
					item.m_onSelected();
				}
			}
			for (auto submenu : b.m_subMenus)
			{
				DoSubMenu(submenu);
			}
			ImGui::EndMenu();
		}
	}

	void DebugGuiSystem::MainMenuBar(MenuBar& bar)
	{
		if (ImGui::BeginMainMenuBar())
		{
			for (auto sm : bar.m_subMenus)
			{
				DoSubMenu(sm);
			}
			ImGui::EndMainMenuBar();
		}
	}

	bool DebugGuiSystem::TreeNode(const char* label, bool forceExpanded)
	{
		if (forceExpanded)
		{
			return ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen);
		}
		else
		{
			return ImGui::TreeNode(label);
		}
	}

	void DebugGuiSystem::TreePop()
	{
		ImGui::TreePop();
	}

	bool DebugGuiSystem::IsCapturingMouse()
	{
		return ImGui::GetIO().WantCaptureMouse;
	}

	bool DebugGuiSystem::IsCapturingKeyboard()
	{
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	void DebugGuiSystem::ItemWidth(float w)
	{
		ImGui::PushItemWidth(w);
	}

	void DebugGuiSystem::SameLine(float xOffset, float spacing)
	{
		ImGui::SameLine(xOffset, spacing);
	}

	bool DebugGuiSystem::BeginWindow(bool& windowOpen, const char* windowName, glm::vec2 size)
	{
		bool ret = ImGui::Begin(windowName, &windowOpen, 0);
		if (size.x > 0 && size.y > 0)
		{
			ImGui::SetWindowSize(ImVec2(size.x, size.y));
		}
		return ret;
	}

	void DebugGuiSystem::EndWindow()
	{
		ImGui::End();
	}

	bool DebugGuiSystem::DragFloat(const char* label, float& f, float step, float min, float max)
	{
		return ImGui::DragFloat(label, &f, step, min, max);
	}

	bool DebugGuiSystem::DragVector(const char* label, glm::vec3& v, float step, float min, float max)
	{
		return ImGui::DragFloat3(label, glm::value_ptr(v), step, min, max);
	}

	bool DebugGuiSystem::ColourEdit(const char* label, glm::vec4& c, bool showAlpha)
	{
		return ImGui::ColorEdit4(label, glm::value_ptr(c), showAlpha);
	}

	bool DebugGuiSystem::DragVector(const char* label, glm::vec4& v, float step, float min, float max)
	{
		return ImGui::DragFloat4(label, glm::value_ptr(v), step, min, max);
	}

	bool DebugGuiSystem::TextInput(const char* label, std::string& str)
	{
		char textBuffer[1024] = { '\0' };
		strcpy_s(textBuffer, str.c_str());
		if (ImGui::InputText(label, textBuffer, sizeof(textBuffer)))
		{
			str = textBuffer;
			return true;
		}
		return false;
	}

	bool DebugGuiSystem::TextInput(const char* label, char* textBuffer, size_t bufferSize)
	{
		return ImGui::InputText(label, textBuffer, bufferSize);
	}

	bool DebugGuiSystem::Selectable(const char* txt, bool selected)
	{
		return ImGui::Selectable(txt, selected);
	}

	bool DebugGuiSystem::Button(const char* txt)
	{
		return ImGui::Button(txt);
	}

	void DebugGuiSystem::Text(const char* txt)
	{
		ImGui::Text(txt);
	}

	void DebugGuiSystem::Separator()
	{
		ImGui::Separator();
	}

	void DebugGuiSystem::GraphLines(const char* label, glm::vec2 size, const std::vector<float>& values)
	{
		ImVec2 graphSize(size.x, size.y);
		ImGui::PlotLines("", values.data(), (int)values.size(), 0, label, FLT_MAX, FLT_MAX, graphSize);
	}

	void DebugGuiSystem::GraphLines(const char* label, glm::vec2 size, GraphDataBuffer& buffer)
	{
		ImVec2 graphSize(size.x, size.y);
		ImGui::PlotLines("", buffer.GetValues(), buffer.ValueCount(), 0, label, FLT_MAX, FLT_MAX, graphSize);
	}

	bool DebugGuiSystem::Checkbox(const char* text, bool* val)
	{
		return ImGui::Checkbox(text, val);
	}

	void DebugGuiSystem::GraphHistogram(const char* label, glm::vec2 size, GraphDataBuffer& buffer)
	{
		ImVec2 graphSize(size.x, size.y);
		ImGui::PlotHistogram("", buffer.GetValues(), buffer.ValueCount(), 0, label, FLT_MAX, FLT_MAX, graphSize);
	}

	void DebugGuiSystem::Image(Render::Texture& src, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1)
	{
		size_t texHandle = static_cast<size_t>(src.GetHandle());
		ImGui::Image(reinterpret_cast<ImTextureID>(texHandle), { size.x,size.y }, { uv0.x, uv0.y }, { uv1.x, uv1.y });
	}

	bool DebugGuiSystem::Tick()
	{
		SDE_PROF_EVENT();

		// Start next frame
		m_imguiPass->NewFrame();
		{
			SDE_PROF_EVENT("ImGui::NewFrame");
			ImGui::NewFrame();
		}

		return true;
	}

	void DebugGuiSystem::Shutdown()
	{
		SDE_PROF_EVENT();

		m_imguiPass = nullptr;
		ImGui::DestroyContext();
	}
}