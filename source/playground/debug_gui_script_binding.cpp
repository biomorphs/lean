#include "debug_gui_script_binding.h"
#include "engine/debug_gui_system.h"
#include <sol.hpp>
#include <string>

namespace DebugGuiScriptBinding
{
	void Go(Engine::DebugGuiSystem* system, sol::state& context)
	{
		auto dbgui = context["DebugGui"].get_or_create<sol::table>();
		dbgui["BeginWindow"] = [system](bool isOpen, std::string title) -> bool {
			return system->BeginWindow(isOpen, title.c_str());
		};
		dbgui["EndWindow"] = [system]() {
			system->EndWindow();
		};
		dbgui["DragFloat"] = [system](std::string label, float v, float step, float min, float max) -> float {
			return system->DragFloat(label.c_str(), v, step, min, max);
		};
		dbgui["Text"] = [system](std::string txt) {
			system->Text(txt.c_str());
		};
		dbgui["TextInput"] = [system](std::string label, std::string currentValue, int maxLength) -> std::string {
			char outBuffer[1024] = { '\0' };
			if (maxLength < 1024 && currentValue.length() < 1024)
			{
				strcpy_s(outBuffer, 1024, currentValue.c_str());
				system->TextInput(label.c_str(), outBuffer, maxLength);
			}
			return outBuffer;
		};
		dbgui["Button"] = [system](std::string txt) -> bool {
			return system->Button(txt.c_str());
		};
		dbgui["Selectable"] = [system](std::string txt, bool selected) -> bool {
			return system->Selectable(txt.c_str(), selected);
		};
		dbgui["Separator"] = [system]() {
			return system->Separator();
		};
		dbgui["Checkbox"] = [system](std::string txt, bool checked) -> bool {
			return system->Checkbox(txt.c_str(), checked);
		};
	}
}