#include "scene_editor.h"
#include "scene.h"
#include "engine/debug_gui_system.h"

void SceneEditor::Init(Scene* scene, Engine::DebugGuiSystem* debugGui)
{
	m_scene = scene;
	m_debugGui = debugGui;
}

void SceneEditor::ToggleEnabled()
{
	m_mainWindowActive = !m_mainWindowActive;
}

bool SceneEditor::Tick()
{
	bool reloadScripts = false;
	char textBuffer[1024] = { '\0' };
	if (m_mainWindowActive)
	{
		m_debugGui->BeginWindow(m_mainWindowActive, "Scene Editor");
		if (m_scene)
		{
			m_debugGui->Text("Scene Name");
			sprintf_s(textBuffer, "%s", m_scene->Name().c_str());
			m_debugGui->SameLine(100.0f);
			m_debugGui->ItemWidth(200.0f);
			m_debugGui->TextInput(" ", m_scene->Name());
		}

		m_debugGui->Separator();
		m_debugGui->Text("Scripts");

		m_debugGui->Text("Add Script");
		m_debugGui->SameLine(100.0f);
		m_debugGui->ItemWidth(165.0f);
		sprintf_s(textBuffer, m_newScriptName.c_str());
		if (m_debugGui->TextInput("", textBuffer, sizeof(textBuffer)))
		{
			m_newScriptName = textBuffer;
		}
		m_debugGui->SameLine();
		if (m_debugGui->Button("Add") && m_newScriptName.length() > 0)
		{
			m_scene->Scripts().push_back(textBuffer);
			m_newScriptName = "";
			reloadScripts = true;
		}

		int index = 0;
		for (auto it : m_scene->Scripts())
		{
			sprintf_s(textBuffer, "X##%d", index++);
			if (m_debugGui->Button(textBuffer))
			{
				m_scene->Scripts().erase(std::find(m_scene->Scripts().begin(), m_scene->Scripts().end(), it));
				reloadScripts = true;
			}
			m_debugGui->SameLine();
			sprintf_s(textBuffer, it.c_str());
			m_debugGui->Text(textBuffer);
		}
		if (m_debugGui->Button("Reload Scripts"))
		{
			reloadScripts = true;
		}
		m_debugGui->EndWindow();
	}
	return reloadScripts;
}