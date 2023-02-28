#include "basic_inspector.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/file_picker_dialog.h"

BasicInspector::BasicInspector()
{
	m_dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
}

void BasicInspector::InspectFilePath(const char* label, std::string_view extension, std::string_view currentVal, std::function<void(std::string)> setValueFn)
{
	std::string btnText = std::string(label) + ": " + std::string(currentVal);
	if (m_dbgGui->Button(btnText.c_str()))
	{
		std::string filePickerFilter = "Files (." + std::string(extension) + ")\0*." + std::string(extension) + "\0";
		std::string path = Engine::ShowFilePicker("Choose file", "", filePickerFilter.c_str(), false);
		if (path != currentVal)
		{
			setValueFn(path);
		}
	}
}

bool BasicInspector::Inspect(const char* label, EntityHandle current, std::function<void(EntityHandle)> setFn, std::function<bool(const EntityHandle&)> filter)
{
	auto entities = Engine::GetSystem<EntitySystem>("Entities");
	static bool selectingEntity = false;
	bool newOneSelected = false;
	std::string btnText = std::string(label) + " : " + (current.GetID() != -1 ? entities->GetEntityNameWithTags(current) : "None");
	if (m_dbgGui->Button(btnText.c_str()))
	{
		selectingEntity = true;
	}
	if (selectingEntity)
	{
		std::string windowLabel = "Select a " + std::string(label);
		if (m_dbgGui->BeginModalPopup(windowLabel.c_str()))
		{
			if (m_dbgGui->Button("None"))
			{
				setFn({});
				selectingEntity = false;
			}
			const auto& allEntities = entities->GetWorld()->AllEntities();
			for (const auto& e : allEntities)
			{
				if (filter(e))
				{
					std::string entityName = entities->GetEntityNameWithTags(e);
					if (m_dbgGui->Button(entityName.c_str()))
					{
						selectingEntity = false;
						if (e != current.GetID())
						{
							setFn({ e });
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

bool BasicInspector::InspectEnum(const char* label, int currentValue, std::function<void(int)> setFn, const char* enumStrs[], int enumStrCount)
{
	int newValue = currentValue;
	if (m_dbgGui->ComboBox(label, enumStrs, enumStrCount, newValue))
	{
		if (newValue != currentValue)
		{
			setFn(newValue);
			return true;
		}
	}
	return false;
}

bool BasicInspector::Inspect(const char* label, bool currentValue, std::function<void(bool)> setFn)
{
	bool newValue = m_dbgGui->Checkbox(label, currentValue);
	if (newValue != currentValue)
	{
		setFn(newValue);
		return true;
	}
	return false;
}

bool BasicInspector::Inspect(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn, float step, float minv, float maxv)
{
	glm::vec3 newValue = m_dbgGui->DragVector(label, currentValue, step, minv, maxv);
	if (newValue != currentValue)
	{
		setFn(newValue);
		return true;
	}
	return false;
}

bool BasicInspector::InspectColour(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn)
{
	glm::vec3 newValue = m_dbgGui->ColourEdit(label, currentValue);
	if (newValue != currentValue)
	{
		setFn(newValue);
		return true;
	}
	return false;
}

bool BasicInspector::Inspect(const char* label, int currentValue, std::function<void(int)> setFn, int step, int minv, int maxv)
{
	int newValue = m_dbgGui->DragInt(label, currentValue, step, minv, maxv);
	if (newValue != currentValue)
	{
		setFn(newValue);
		return true;
	}
	return false;
}

bool BasicInspector::Inspect(const char* label, float currentValue, std::function<void(float)> setFn, float step, float minv, float maxv)
{
	float newValue = m_dbgGui->DragFloat(label, currentValue, step, minv, maxv);
	if (newValue != currentValue)
	{
		setFn(newValue);
		return true;
	}
	return false;
}

bool BasicInspector::Inspect(const char* label, double currentValue, std::function<void(double)> setFn, double step, double minv, double maxv)
{
	float newValue = m_dbgGui->DragFloat(label, (float)currentValue, (float)step, (float)minv, (float)maxv);
	if (newValue != currentValue)
	{
		setFn(newValue);
		return true;
	}
	return false;
}