#include "editor_component_inspector.h"
#include "editor.h"	
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "commands/editor_set_value_cmd.h"

EditorComponentInspector::EditorComponentInspector()
{
	m_dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	m_editor = Engine::GetSystem<Editor>("Editor");
}

bool EditorComponentInspector::Inspect(const char* label, EntityHandle current, std::function<void(EntityHandle)> setFn, std::function<bool(const EntityHandle&)> filter)
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	static std::string selectingLabel;
	bool newOneSelected = false;
	std::string btnText = std::string(label) + " : " + (current.GetID() != -1 ? entities->GetEntityNameWithTags(current) : "None");
	if (m_dbgGui->Button(btnText.c_str()))
	{
		selectingLabel = label;
	}
	if (selectingLabel == label)
	{
		const auto& allEntities = entities->GetWorld()->AllEntities();
		std::string windowLabel = "Select " + std::string(label);
		if (m_dbgGui->BeginModalPopup(windowLabel.c_str()))
		{
			if (m_dbgGui->Button("None"))
			{
				m_editor->PushCommand(std::make_unique<EditorSetValueCommand<EntityHandle>>(label, current, EntityHandle(), setFn));
				selectingLabel = "";
			}
			for (const auto& e : allEntities)
			{
				if (filter(e))
				{
					std::string entityName = entities->GetEntityNameWithTags(e);
					if (m_dbgGui->Button(entityName.c_str()))
					{
						selectingLabel = "";
						if (e != current.GetID())
						{
							m_editor->PushCommand(std::make_unique<EditorSetValueCommand<EntityHandle>>(label, current, e, setFn));
							newOneSelected = true;
						}
					}
				}
			}
			m_dbgGui->EndModalPopup();
		}
	}
	return newOneSelected;
}

bool EditorComponentInspector::InspectEnum(const char* label, int currentValue, std::function<void(int)> setFn, const char* enumStrs[], int enumStrCount)
{
	m_dbgGui->AlignToNextControl();
	m_dbgGui->Text(label);
	m_dbgGui->SameLine(m_labelWidth);
	std::string inputId = std::string("##") + label + "_value";
	int newValue = currentValue;
	if (m_dbgGui->ComboBox(inputId.c_str(), enumStrs, enumStrCount, newValue))
	{
		if (newValue != currentValue)
		{
			m_editor->PushCommand(std::make_unique<EditorSetValueCommand<int>>(label, currentValue, newValue, setFn));
			return true;
		}
	}
	return false;
}

bool EditorComponentInspector::Inspect(const char* label, bool currentValue, std::function<void(bool)> setFn)
{
	m_dbgGui->AlignToNextControl();
	m_dbgGui->Text(label);
	m_dbgGui->SameLine(m_labelWidth);
	std::string inputId = std::string("##") + label + "_value";
	bool newValue = m_dbgGui->Checkbox(inputId.c_str(), currentValue);
	if (newValue != currentValue)
	{
		m_editor->PushCommand(std::make_unique<EditorSetValueCommand<bool>>(label, currentValue, newValue, setFn));
		return true;
	}
	return false;
}

bool EditorComponentInspector::Inspect(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn, float step, float minv, float maxv)
{
	m_dbgGui->AlignToNextControl();
	m_dbgGui->Text(label);
	m_dbgGui->SameLine(m_labelWidth);
	std::string inputId = std::string("##") + label + "_value";
	glm::vec3 newValue = currentValue; //m_dbgGui->DragVector(inputId.c_str(), currentValue, step, minv, maxv);
	m_dbgGui->PushItemWidth(120.0f);
	newValue.x = m_dbgGui->InputFloat((inputId + "_x").c_str(), currentValue.x, step, minv, maxv);
	m_dbgGui->SameLine();
	newValue.y = m_dbgGui->InputFloat((inputId + "_y").c_str(), currentValue.y, step, minv, maxv);
	m_dbgGui->SameLine();
	newValue.z = m_dbgGui->InputFloat((inputId + "_z").c_str(), currentValue.z, step, minv, maxv);
	m_dbgGui->PopItemWidth();
	if (newValue != currentValue)
	{
		m_editor->PushCommand(std::make_unique<EditorSetValueCommand<glm::vec3>>(label, currentValue, newValue, setFn));
		return true;
	}
	return false;
}

bool EditorComponentInspector::InspectColour(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn)
{
	m_dbgGui->AlignToNextControl();
	m_dbgGui->Text(label);
	m_dbgGui->SameLine(m_labelWidth);
	std::string inputId = std::string("##") + label + "_value";
	glm::vec3 newValue = m_dbgGui->ColourEdit(inputId.c_str(), currentValue);
	if (newValue != currentValue)
	{
		m_editor->PushCommand(std::make_unique<EditorSetValueCommand<glm::vec3>>(label, currentValue, newValue, setFn));
		return true;
	}
	return false;
}

bool EditorComponentInspector::Inspect(const char* label, int currentValue, std::function<void(int)> setFn, int step, int minv, int maxv)
{
	m_dbgGui->AlignToNextControl();
	m_dbgGui->Text(label);
	m_dbgGui->SameLine(m_labelWidth);
	std::string inputId = std::string("##") + label + "_value";
	int newValue = m_dbgGui->InputInt(inputId.c_str(), currentValue, step, minv, maxv);
	if (newValue != currentValue)
	{
		m_editor->PushCommand(std::make_unique<EditorSetValueCommand<int>>(label, currentValue, newValue, setFn));
		return true;
	}
	return false;
}

bool EditorComponentInspector::Inspect(const char* label, float currentValue, std::function<void(float)> setFn, float step, float minv, float maxv)
{
	m_dbgGui->AlignToNextControl();
	m_dbgGui->Text(label);
	m_dbgGui->SameLine(m_labelWidth);
	std::string inputId = std::string("##") + label + "_value";
	float newValue = m_dbgGui->InputFloat(inputId.c_str(), currentValue, step, minv, maxv);
	if (newValue != currentValue)
	{
		m_editor->PushCommand(std::make_unique<EditorSetValueCommand<float>>(label, currentValue, newValue, setFn));
		return true;
	}
	return false;
}