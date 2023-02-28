#pragma once

#include "entity/component_inspector.h"

namespace Engine
{
	class DebugGuiSystem;
}

class Editor;
class EditorComponentInspector : public ComponentInspector
{
public:
	EditorComponentInspector();
	virtual bool Inspect(const char* label, EntityHandle current, std::function<void(EntityHandle)> setFn, std::function<bool(const EntityHandle&)> filter);
	virtual bool InspectEnum(const char* label, int currentValue, std::function<void(int)> setFn, const char* enumStrs[], int enumStrCount);
	virtual bool Inspect(const char* label, bool currentValue, std::function<void(bool)> setFn);
	virtual bool Inspect(const char* label, int currentValue, std::function<void(int)> setFn, int step, int minv, int maxv);
	virtual bool Inspect(const char* label, float currentValue, std::function<void(float)> setFn, float step, float minv, float maxv);
	virtual bool Inspect(const char* label, double currentValue, std::function<void(double)> setFn, double step, double minv, double maxv);
	virtual bool Inspect(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn, float step, float minv, float maxv);
	virtual bool InspectColour(const char* label, glm::vec3 currentValue, std::function<void(glm::vec3)> setFn);
	void InspectFilePath(const char* label, std::string_view extension, std::string_view currentVal, std::function<void(std::string)> setValueFn);
private:
	Editor* m_editor = nullptr;
	Engine::DebugGuiSystem* m_dbgGui = nullptr;
	float m_labelWidth = 200.0f;
};