#include "emitter_editor.h"
#include "emitter_descriptor.h"
#include "particle_emission_behaviour.h"
#include "editor_value_inspector.h"
#include "particle_system.h"
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
#include "behaviours/generate_random_position.h"
#include "behaviours/generate_random_velocity.h"
#include "behaviours/debug_axis_renderer.h"
#include "behaviours/euler_position_update.h"
#include "behaviours/gravity_update.h"

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

	template<class BV>
	class NewBehaviourCmd : public Command {
	public:
		int m_targetEmitter;
		std::unique_ptr<BV> m_behaviour;

		const char* GetName() { return "New Behaviour"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			editor->AddBehaviourToEmitter(m_targetEmitter, std::move(m_behaviour));
			return Command::Result::Succeeded;
		}
	};

	class DeleteBehaviourCmd : public Command {
	public:
		int m_targetEmitter;
		int m_index;
		EmitterEditor::BehaviourType m_type;
		const char* GetName() { return "Delete Behaviour"; }
		bool CanUndoRedo() { return false; }
		Result Execute() {
			auto editor = Engine::GetSystem<EmitterEditor>("EmitterEditor");
			editor->DeleteBehaviour(m_type, m_targetEmitter, m_index);
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
		SDE_PROF_EVENT();
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

		RegisterBehaviour(std::make_unique<EmitOnce>());
		RegisterBehaviour(std::make_unique<EmitBurstRepeater>());
		RegisterBehaviour(std::make_unique<GenerateRandomPosition>());
		RegisterBehaviour(std::make_unique<GenerateRandomVelocity>());
		RegisterBehaviour(std::make_unique<DebugAxisRenderer>());
		RegisterBehaviour(std::make_unique<EulerPositionUpdater>());
		RegisterBehaviour(std::make_unique<GravityUpdate>());
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
		SDE_PROF_EVENT();
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
		SDE_PROF_EVENT();
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

			// let the particle system know this one changed
			auto particles = Engine::GetSystem<ParticleSystem>("Particles");
			particles->InvalidateEmitter(path);

			return result;
		}
		return false;
	}

	void EmitterEditor::DrawTabButtons(Engine::DebugGuiSystem& gui)
	{
		SDE_PROF_EVENT();
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

	template<class BV>
	void InspectBehaviours(Engine::DebugGuiSystem& gui, int emitterID, const char* label, EmitterEditor::BehaviourType type, 
		std::vector<std::unique_ptr<BV>>& bvs, std::unordered_map<std::string, std::unique_ptr<BV>>& allBvs, EditorValueInspector& i)
	{
		SDE_PROF_EVENT();
		auto mainEditor = Engine::GetSystem<Editor>("Editor");
		gui.Text(label);
		for (int b = 0; b < bvs.size(); ++b)
		{
			gui.Text(bvs[b]->GetName().data());
			bvs[b]->Inspect(i);
			std::string deleteBtnText = "Delete?##" + std::to_string(b);
			if (gui.Button(deleteBtnText.c_str()))
			{
				auto cmd = std::make_unique<DeleteBehaviourCmd>();
				cmd->m_targetEmitter = emitterID;
				cmd->m_index = b;
				cmd->m_type = type;
				mainEditor->PushCommand(std::move(cmd));
			}
			gui.Separator();
		}
		std::vector<std::string> allBehaviours;
		for (auto it = allBvs.begin(); it != allBvs.end(); ++it)
		{
			allBehaviours.push_back(it->first);
		}
		int currentItem = 0;
		std::string lbl = "Add " + std::string(label) + " Behaviour";
		if (gui.ComboBox(lbl.c_str(), allBehaviours, currentItem))
		{
			auto foundBv = allBvs.find(allBehaviours[currentItem]);
			if (foundBv != allBvs.end())
			{
				auto nbh = std::make_unique<NewBehaviourCmd<BV>>();
				nbh->m_targetEmitter = emitterID;
				nbh->m_behaviour = std::move(foundBv->second->MakeNew());
				mainEditor->PushCommand(std::move(nbh));
			}
		}
		gui.Separator();
	}

	void EmitterEditor::ShowCurrentEmitter(Engine::DebugGuiSystem& gui)
	{
		SDE_PROF_EVENT();
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
		inspector.Inspect("Max Particles", activeEmitter->m_emitter->GetMaxParticles(), [activeEmitter](int v) {
			activeEmitter->m_emitter->SetMaxParticles(v);
		});

		InspectBehaviours(gui, m_currentEmitterID, "Emitter", BehaviourType::Emitter, activeEmitter->m_emitter->GetEmissionBehaviours(), m_emissionBehaviours, inspector);
		InspectBehaviours(gui, m_currentEmitterID, "Generator", BehaviourType::Generator, activeEmitter->m_emitter->GetGenerators(), m_generatorBehaviours, inspector);
		InspectBehaviours(gui, m_currentEmitterID, "Updater", BehaviourType::Updater, activeEmitter->m_emitter->GetUpdaters(), m_updateBehaviours, inspector);
		InspectBehaviours(gui, m_currentEmitterID, "Renderer", BehaviourType::Renderer, activeEmitter->m_emitter->GetRenderers(), m_renderBehaviours, inspector);
	}

	EmitterEditor::ActiveEmitter* EmitterEditor::FindEmitter(int id)
	{
		auto found = std::find_if(m_activeEmitters.begin(), m_activeEmitters.end(), [id](const ActiveEmitter& a) {
			return a.m_id == id;
		});
		return found == m_activeEmitters.end() ? nullptr : &(*found);
	}

	void EmitterEditor::DeleteBehaviour(BehaviourType type, int emitterID, int behaviourIndex)
	{
		SDE_PROF_EVENT();
		auto em = FindEmitter(emitterID);
		if (em && behaviourIndex >= 0)
		{
			switch (type)
			{
			case Emitter:
				if (behaviourIndex < em->m_emitter->GetEmissionBehaviours().size())
				{
					em->m_emitter->GetEmissionBehaviours().erase(em->m_emitter->GetEmissionBehaviours().begin() + behaviourIndex);
				}
				break;
			case Generator:
				if (behaviourIndex < em->m_emitter->GetGenerators().size())
				{
					em->m_emitter->GetGenerators().erase(em->m_emitter->GetGenerators().begin() + behaviourIndex);
				}
				break;
			case Updater:
				if (behaviourIndex < em->m_emitter->GetUpdaters().size())
				{
					em->m_emitter->GetUpdaters().erase(em->m_emitter->GetUpdaters().begin() + behaviourIndex);
				}
				break;
			case Renderer:
				if (behaviourIndex < em->m_emitter->GetRenderers().size())
				{
					em->m_emitter->GetRenderers().erase(em->m_emitter->GetRenderers().begin() + behaviourIndex);
				}
				break;
			default:
				assert(false);
			}
		}
	}

	int EmitterEditor::NewEmitter()
	{
		SDE_PROF_EVENT();
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
		SDE_PROF_EVENT();
		const char* c_windowName = "Emitter Editor";
		auto dbgGui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
		if (m_windowOpen)
		{
			dbgGui->BeginWindow(m_windowOpen, c_windowName, Engine::GuiWindowFlags::HasMenuBar);
			dbgGui->WindowMenuBar(m_menuBar);
			DrawTabButtons(*dbgGui);
			ShowCurrentEmitter(*dbgGui);
			dbgGui->EndWindow();
		}
		return true;
	}
}