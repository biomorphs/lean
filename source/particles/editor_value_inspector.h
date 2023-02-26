#pragma once
#include <string>
#include <functional>
#include "core/glm_headers.h"

namespace Engine
{
	class DebugGuiSystem;
}

namespace Particles
{
	class EditorValueInspector
	{
	public:
		Engine::DebugGuiSystem* m_dbgGui = nullptr;
		int m_lblIndexCounter = 0;	// to keep imgui happy with multiple inspectors with same name
		void Inspect(std::string_view label, std::string_view currentVal, std::function<void(std::string_view)> setValueFn);
		void Inspect(std::string_view label, int currentVal, std::function<void(int)> setValueFn);
		void Inspect(std::string_view label, bool currentVal, std::function<void(bool)> setValueFn);
		void Inspect(std::string_view label, float currentVal, std::function<void(float)> setValueFn);
		void Inspect(std::string_view label, glm::vec3 currentVal, std::function<void(glm::vec3)> setValueFn);
		void InspectFilePath(std::string_view label, std::string_view extension, std::string_view currentVal, std::function<void(std::string_view)> setValueFn);
	};
}