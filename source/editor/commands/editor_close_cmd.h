#pragma once

#include "editor/command.h"

namespace Engine
{
	class DebugGuiSystem;
}

class Editor;
class EditorCloseCommand : public Command
{
public:
	EditorCloseCommand(Engine::DebugGuiSystem* dbgGui, Editor* editor)
		: m_gui(dbgGui)
		, m_editor(editor)
	{
	}
	virtual const char* GetName() { return "Close Editor"; }
	virtual Result Execute();
	virtual bool CanUndo() { return false; }

private:
	Engine::DebugGuiSystem* m_gui = nullptr;
	Editor* m_editor = nullptr;
};