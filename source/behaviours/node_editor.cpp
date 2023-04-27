#include "node_editor.h"
#include "node.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"

namespace Behaviours
{
	void ShowPinEditor(int n, Engine::DebugGuiSystem& gui, PinProperties& p)
	{
		std::string postFix = "##" + std::to_string(n);
		std::string txt = "Output Pin #" + std::to_string(n) + postFix;
		gui.Text(txt.data());
		txt = "Colour" + postFix;
		p.m_colour = gui.ColourEdit(txt.data(), p.m_colour);
		txt = "Annotation" + postFix;
		gui.TextInput(txt.data(), p.m_annotation);
		txt = "Connected Node" + postFix;
		p.m_connectedNodeID = gui.InputInt(txt.data(), p.m_connectedNodeID, 1, -1, UINT16_MAX);
	}

	bool NodeEditor::ShowEditor(Node* n)
	{
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		bool windowOpen = true;
		if (dbgGui->BeginWindow(windowOpen, "Node Editor"))
		{
			std::string txt = "Local ID: " + std::to_string(n->m_localID);
			dbgGui->Text(txt.c_str());
			dbgGui->TextInput("Node Name", n->m_name);
			dbgGui->Separator();
			n->ShowEditorGui(*dbgGui);
			dbgGui->Separator();
			n->m_bgColour = dbgGui->ColourEdit("BG Colour", n->m_bgColour);
			n->m_textColour = dbgGui->ColourEdit("Text Colour", n->m_textColour);
			n->m_editorPosition = dbgGui->DragVector("Position", n->m_editorPosition, 1.0f, 0.0f, 10000.0f);
			n->m_editorDimensions = dbgGui->DragVector("Dimensions", n->m_editorDimensions, 1.0f, 16.0f, 512.0f);
			dbgGui->Separator();
			for (int i = 0; i < n->m_outputPins.size(); ++i)
			{
				dbgGui->Separator();
				ShowPinEditor(i, *dbgGui, n->m_outputPins[i]);
			}
			dbgGui->EndWindow();
		}
		return windowOpen;
	}
}