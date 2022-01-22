#pragma once

#include <string>
#include "editor/command.h"

namespace Engine
{
	class DebugGuiSystem;
}

class Editor;
class EditorSaveSceneCommand : public Command
{
public:
	EditorSaveSceneCommand(Engine::DebugGuiSystem* dbgGui, Editor* editor, std::string targetPath)
		: m_gui(dbgGui)
		, m_editor(editor)
		, m_targetPath(targetPath)
	{
	}
	virtual const char* GetName() { return "Save Scene"; }
	virtual Result Execute();
	virtual bool CanUndo() { return false; }

private:
	Engine::DebugGuiSystem* m_gui = nullptr;
	Editor* m_editor = nullptr;
	std::string m_targetPath = "";
};