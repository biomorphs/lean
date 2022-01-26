#pragma once

#include <string>
#include "editor/command.h"

namespace Engine
{
	class DebugGuiSystem;
}

class Editor;
class EditorImportSceneCommand : public Command
{
public:
	EditorImportSceneCommand(Editor* editor)
		: m_editor(editor)
	{
	}
	virtual const char* GetName() { return "Import Scene"; }
	virtual Result Execute();
	virtual bool CanUndoRedo() { return false; }

private:
	Editor* m_editor = nullptr;
};