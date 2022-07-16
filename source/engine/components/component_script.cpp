#include "component_script.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"
#include "engine/file_picker_dialog.h"
#include "entity/entity_handle.h"
#include "core/base64.h"

COMPONENT_SCRIPTS(Script,
	"SetNeedsCompile", &Script::SetNeedsCompile
)

SERIALISE_BEGIN(Script)
SERIALISE_PROPERTY("ScriptFile", m_scriptFile)
if (op == Engine::SerialiseType::Read)
{
	std::string encoded;
	SERIALISE_PROPERTY("ScriptText", encoded);
	m_scriptText = base64_decode(encoded);
	m_needsCompile = true;
}
else
{
	std::string encoded = base64_encode(m_scriptText);
	SERIALISE_PROPERTY("ScriptText", encoded);
}
SERIALISE_END()

COMPONENT_INSPECTOR_IMPL(Script)
{
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	auto fn = [gui](ComponentInspector& i, ComponentStorage& cs, const EntityHandle& e)
	{
		auto& s = *static_cast<StorageType&>(cs).Find(e);
		static EntityHandle editStringHandle;
		static std::string editString;
		std::string scriptFile = s.GetScriptFile();
		if (gui->Button(scriptFile.length() > 0 ? scriptFile.c_str() : "Select a file"))
		{
			std::string newFile = Engine::ShowFilePicker("Select Script", "", "Lua script (.lua)\0*.lua\0");
			if (newFile != "")
			{
				s.SetFile(newFile);
			}
		}
		if (gui->Button("Edit"))
		{
			editStringHandle = e;
			editString = s.GetFunctionText();
		}
		if (editStringHandle == e)
		{
			gui->TextInputMultiline("Script", { 512, 512 }, editString);
			if (gui->Button("Ok"))
			{
				s.SetFunctionText(editString);
				editString = "";
				editStringHandle = {};
			}
			gui->SameLine();
			if (gui->Button("Cancel"))
			{
				editString = "";
				editStringHandle = {};
			}
		}
		else
		{
			gui->Text(s.GetFunctionText().c_str());
		}
	};
	return fn;
}