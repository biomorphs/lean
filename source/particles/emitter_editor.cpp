#include "emitter_editor.h"
#include "emitter_descriptor.h"
#include "particle_emission_behaviour.h"
#include "editor_value_inspector.h"
#include "core/file_io.h"
#include "engine/system_manager.h"
#include "engine/debug_gui_system.h"
#include "engine/debug_gui_menubar.h"
#include "engine/file_picker_dialog.h"
#include "editor/command.h"
#include "editor/editor.h"
#include "editor/commands/editor_set_value_cmd.h"

#include "behaviours/emit_burst_repeater.h"
#include "behaviours/emit_once.h"

namespace Particles
{
	class NewEmitterCmd : public Command {
		const char* GetName() { return "New Emitter"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			editor->NewEmitter();
			return Command::Result::Succeeded;
		}
	};

	class NewEmissionBehaviourCmd : public Command {
	public:
		int m_targetEmitter;
		std::unique_ptr<EmissionBehaviour> m_behaviour;

		const char* GetName() { return "New Emission Behaviour"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			editor->AddEmissionBehaviour(m_targetEmitter, std::move(m_behaviour));
			return Command::Result::Succeeded;
		}
	};

	class DeleteEmissionBehaviourCmd : public Command {
	public:
		int m_targetEmitter;
		int m_index;
		const char* GetName() { return "Delete Emission Behaviour"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			editor->DeleteEmissionBehaviour(m_targetEmitter, m_index);
			return Command::Result::Succeeded;
		}
	};

	class SaveEmitterCmd : public Command {
	public:
		int m_targetEmitter;
		std::string m_targetPath;
		const char* GetName() { return "Save Emitter"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			std::string targetPath = m_targetPath;
			if (targetPath == "")
			{
				targetPath = Engine::ShowFilePicker("Save Emitter", "","Emitter Files (.emit)\0*.emit\0", true);
			}
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			bool success = editor->SaveEmitter(m_targetEmitter, targetPath);
			return success ? Command::Result::Succeeded : Command::Result::Failed;
		}
	};

	class LoadEmitterCmd : public Command {
	public:
		const char* GetName() { return "Load Emitter"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			std::string targetPath = Engine::ShowFilePicker("Load Emitter", "", "Emitter Files (.emit)\0*.emit\0");
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			bool success = targetPath != "" ? editor->LoadEmitter(targetPath) : true;
			return success ? Command::Result::Succeeded : Command::Result::Failed;
		}
	};

	EmitterEditor::EmitterEditor()
	{	
		auto& fileMenu = m_menuBar.AddSubmenu("File");
		fileMenu.AddItem("New Emitter", []() {
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto newEmitterCmd = std::make_unique<NewEmitterCmd>();
			mainEditor->PushCommand(std::move(newEmitterCmd));
		});
		fileMenu.AddItem("Load Emitter", []() {
			auto mainEditor = Engine::GetSystem<Editor>("Editor");
			auto cmd = std::make_unique<LoadEmitterCmd>();
			mainEditor->PushCommand(std::move(cmd));
		});
		fileMenu.AddItem("Save", [this]() {
			auto emt = FindEmitter(m_currentEmitterID);
			if (emt)
			{
				auto cmd = std::make_unique<SaveEmitterCmd>();
				cmd->m_targetEmitter = m_currentEmitterID;
				cmd->m_targetPath = emt->m_documentPath;
				auto mainEditor = Engine::GetSystem<Editor>("Editor");
				mainEditor->PushCommand(std::move(cmd));
			}
		});
		fileMenu.AddItem("Close", [this]() {
			DoOnClose();
		});

		RegisterEmissionBehaviour(std::make_unique<EmitOnce>());
		RegisterEmissionBehaviour(std::make_unique<EmitBurstRepeater>());
	}

	EmitterEditor::~EmitterEditor()
	{
	}

	void EmitterEditor::DoOnClose()
	{
		m_windowOpen = false;
	}

	int EmitterEditor::LoadEmitter(std::string_view path)
	{
		std::string fileText;
		if (!Core::LoadTextFromFile(path.data(), fileText))
		{
			return -1;
		}

		nlohmann::json emitterJson;
		{
			SDE_PROF_EVENT("ParseJson");
			emitterJson = nlohmann::json::parse(fileText);
		}

		int newID = m_nextActiveEmitterId++;

		ActiveEmitter newEmitterDoc;
		newEmitterDoc.m_emitter = std::make_unique<EmitterDescriptor>();
		newEmitterDoc.m_emitter->Serialise(emitterJson, Engine::SerialiseType::Read);
		newEmitterDoc.m_documentPath = path;
		newEmitterDoc.m_id = newID;

		m_activeEmitters.emplace_back(std::move(newEmitterDoc));
		m_currentEmitterID = newID;

		return newID;
	}

	bool EmitterEditor::SaveEmitter(int ID, std::string_view path)
	{
		auto foundEmt = FindEmitter(ID);
		if (foundEmt)
		{
			nlohmann::json emitterData;
			foundEmt->m_emitter->Serialise(emitterData, Engine::SerialiseType::Write);
			bool result = Core::SaveTextToFile(path.data(), emitterData.dump(2));
			if (result)
			{
				foundEmt->m_documentPath = path;
			}
			return result;
		}
		return false;
	}

	void EmitterEditor::DrawTabButtons(Engine::DebugGuiSystem& gui)
	{
		for (int emitter = 0; emitter < m_activeEmitters.size(); ++emitter)
		{
			const auto& em = m_activeEmitters[emitter];
			std::string tabText = std::string(em.m_emitter->GetName());
			tabText += "(" + em.m_documentPath + ")##" + std::to_string(emitter);
			if (gui.Button(tabText.c_str()))
			{
				m_currentEmitterID = m_activeEmitters[emitter].m_id;
			}
			if (emitter + 1 < m_activeEmitters.size())
			{
				gui.SameLine();
			}
		}
	}

	void EmitterEditor::ShowCurrentEmitter(Engine::DebugGuiSystem& gui)
	{
		auto activeEmitter = FindEmitter(m_currentEmitterID);
		if (!activeEmitter)
		{
			return;
		}
		gui.Separator();

		auto mainEditor = Engine::GetSystem<Editor>("Editor");
		EditorValueInspector inspector;
		inspector.Inspect("Emitter Name", activeEmitter->m_emitter->GetName(), [activeEmitter](std::string_view n) {
			activeEmitter->m_emitter->SetName(n);
		});

		gui.Text("Emission Behaviours:");
		auto& bvs = activeEmitter->m_emitter->GetEmissionBehaviours();
		for (int b = 0; b < bvs.size(); ++b)
		{
			gui.Text(bvs[b]->GetName().data());
			bvs[b]->Inspect(inspector);
			std::string deleteBtnText = "Delete?##" + std::to_string(b);
			if (gui.Button(deleteBtnText.c_str()))
			{
				auto cmd = std::make_unique<DeleteEmissionBehaviourCmd>();
				cmd->m_targetEmitter = m_currentEmitterID;
				cmd->m_index = b;
				mainEditor->PushCommand(std::move(cmd));
			}
			gui.Separator();
		}

		std::vector<std::string> allBehaviours;
		for (auto it = m_emissionBehaviours.begin(); it != m_emissionBehaviours.end(); ++it)
		{
			allBehaviours.push_back(it->first);
		}
		int currentItem = 0;
		if (gui.ComboBox("Add Emission Behaviour", allBehaviours, currentItem))
		{
			auto foundBv = m_emissionBehaviours.find(allBehaviours[currentItem]);
			if (foundBv != m_emissionBehaviours.end())
			{
				auto nbh = std::make_unique<NewEmissionBehaviourCmd>();
				nbh->m_targetEmitter = m_currentEmitterID;
				nbh->m_behaviour = foundBv->second->MakeNew();
				mainEditor->PushCommand(std::move(nbh));
			}
		}
	}

	void EmitterEditor::RegisterEmissionBehaviour(std::unique_ptr<EmissionBehaviour>&& b)
	{
		m_emissionBehaviours[std::string(b->GetName())] = std::move(b);
	}

	EmitterEditor::ActiveEmitter* EmitterEditor::FindEmitter(int id)
	{
		auto found = std::find_if(m_activeEmitters.begin(), m_activeEmitters.end(), [id](const ActiveEmitter& a) {
			return a.m_id == id;
		});
		return found == m_activeEmitters.end() ? nullptr : &(*found);
	}

	void EmitterEditor::AddEmissionBehaviour(int emitterID, std::unique_ptr<EmissionBehaviour>&& b)
	{
		auto em = FindEmitter(emitterID);
		if(em)
		{
			em->m_emitter->GetEmissionBehaviours().emplace_back(std::move(b));
		}
	}

	void EmitterEditor::DeleteEmissionBehaviour(int emitterID, int behaviourIndex)
	{
		auto em = FindEmitter(emitterID);
		if (em)
		{
			auto& bv = em->m_emitter->GetEmissionBehaviours();
			if (behaviourIndex < bv.size() && behaviourIndex >= 0)
			{
				bv.erase(bv.begin() + behaviourIndex);
			}
		}
	}

	int EmitterEditor::NewEmitter()
	{
		int newID = m_nextActiveEmitterId++;

		ActiveEmitter newEmitterDoc;
		newEmitterDoc.m_emitter = std::make_unique<EmitterDescriptor>();
		newEmitterDoc.m_emitter->SetName("New Emitter");
		newEmitterDoc.m_documentPath = "";
		newEmitterDoc.m_id = newID;
		
		m_activeEmitters.emplace_back(std::move(newEmitterDoc));
		m_currentEmitterID = newID;

		return newID;
	}

	bool EmitterEditor::Tick(float timeDelta)
	{
		const char* c_windowName = "Emitter Editor";
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		if (m_windowOpen)
		{
			if (dbgGui->BeginWindow(m_windowOpen, c_windowName, Engine::GuiWindowFlags::HasMenuBar))
			{
				dbgGui->WindowMenuBar(m_menuBar);
				DrawTabButtons(*dbgGui);
				ShowCurrentEmitter(*dbgGui);
				dbgGui->EndWindow();
			}
		}
		return true;
	}
}