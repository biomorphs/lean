#include "editor_value_inspector.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "editor/editor.h"
#include "editor/commands/editor_set_value_cmd.h"
#include "engine/file_picker_dialog.h"

namespace Particles
{
	void EditorValueInspector::InspectFilePath(std::string_view label, std::string_view extension, std::string_view currentVal, std::function<void(std::string_view)> setValueFn)
	{
		std::string btnText = std::string(label) + ": " + std::string(currentVal);
		if (m_dbgGui->Button(btnText.c_str()))
		{
			std::string filePickerFilter = "Files (." + std::string(extension) + ")\0*." + std::string(extension) + "\0";
			std::string path = Engine::ShowFilePicker("Choose file", "", filePickerFilter.c_str(), false);
			if (path != currentVal)
			{
				auto mainEditor = Engine::GetSystem<Editor>("Editor");
				auto setValueCmd = std::make_unique<EditorSetValueCommand<std::string>>(label.data(), std::string(currentVal), path, setValueFn);
				mainEditor->PushCommand(std::move(setValueCmd));
			}
		}
	}

	void EditorValueInspector::Inspect(std::string_view label, std::string_view currentVal, std::function<void(std::string_view)> setValueFn)
	{
		std::string oldValue(currentVal);
		std::string val = oldValue;
		std::string lblText = std::string(label) + "##" + std::to_string(m_lblIndexCounter++);
		if (m_dbgGui->TextInput(lblText.data(), val) && val != oldValue)
		{
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto setValueCmd = std::make_unique<EditorSetValueCommand<std::string>>(label.data(), oldValue, val, setValueFn);
			mainEditor->PushCommand(std::move(setValueCmd));
		}
	}

	void EditorValueInspector::Inspect(std::string_view label, bool currentVal, std::function<void(bool)> setValueFn)
	{
		std::string lblText = std::string(label) + "##" + std::to_string(m_lblIndexCounter++);
		bool val = m_dbgGui->Checkbox(lblText.data(), currentVal);
		if (val != currentVal)
		{
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto setValueCmd = std::make_unique<EditorSetValueCommand<bool>>(label.data(), currentVal, val, setValueFn);
			mainEditor->PushCommand(std::move(setValueCmd));
		}
	}

	void EditorValueInspector::Inspect(std::string_view label, int currentVal, std::function<void(int)> setValueFn)
	{
		std::string lblText = std::string(label) + "##" + std::to_string(m_lblIndexCounter++);
		int val = m_dbgGui->InputInt(lblText.data(), currentVal);
		if (val != currentVal)
		{
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto setValueCmd = std::make_unique<EditorSetValueCommand<int>>(label.data(), currentVal, val, setValueFn);
			mainEditor->PushCommand(std::move(setValueCmd));
		}
	}

	void EditorValueInspector::Inspect(std::string_view label, float currentVal, std::function<void(float)> setValueFn)
	{
		std::string lblText = std::string(label) + "##" + std::to_string(m_lblIndexCounter++);
		float val = m_dbgGui->InputFloat(lblText.data(), currentVal, 0.1f);
		if (val != currentVal)
		{
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto setValueCmd = std::make_unique<EditorSetValueCommand<float>>(label.data(), currentVal, val, setValueFn);
			mainEditor->PushCommand(std::move(setValueCmd));
		}
	}

	void EditorValueInspector::Inspect(std::string_view label, glm::vec3 currentVal, std::function<void(glm::vec3)> setValueFn)
	{
		m_dbgGui->AlignToNextControl();
		m_dbgGui->Text(label.data());
		m_dbgGui->SameLine(100);
		std::string inputId = "## " + std::string(label) + "_value";
		glm::vec3 newValue = currentVal;
		m_dbgGui->PushItemWidth(100.0f);
		newValue.x = m_dbgGui->InputFloat((inputId + "_x").c_str(), currentVal.x, 0.1f);
		m_dbgGui->SameLine();
		newValue.y = m_dbgGui->InputFloat((inputId + "_y").c_str(), currentVal.y, 0.1f);
		m_dbgGui->SameLine();
		newValue.z = m_dbgGui->InputFloat((inputId + "_z").c_str(), currentVal.z, 0.1f);
		m_dbgGui->PopItemWidth();
		if (newValue != currentVal)
		{
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto setValueCmd = std::make_unique<EditorSetValueCommand<glm::vec3>>(label.data(), currentVal, newValue, setValueFn);
			mainEditor->PushCommand(std::move(setValueCmd));
		}
	}
}