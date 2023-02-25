#include "editor_value_inspector.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "editor/editor.h"
#include "editor/commands/editor_set_value_cmd.h"

namespace Particles
{
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
}