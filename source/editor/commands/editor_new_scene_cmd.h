#pragma once

#include <string>
#include "editor/command.h"

namespace Engine
{
	class DebugGuiSystem;
}

class Editor;
class EditorNewSceneCommand : public Command
{
public:
	EditorNewSceneCommand(Engine::DebugGuiSystem* dbgGui, Editor* editor)
		: m_gui(dbgGui)
		, m_editor(editor)
	{
	}
	virtual const char* GetName() { return "New Scene"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return false; }

private:
	Engine::DebugGuiSystem* m_gui = nullptr;
	Editor* m_editor = nullptr;
	std::string m_newSceneName = "New Scene";
};