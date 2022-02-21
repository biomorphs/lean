#include "command_list.h"
#include "command.h"
#include "core/log.h"
#include "engine/debug_gui_system.h"
#include "engine/system_manager.h"

bool CommandList::HasWork()
{
	return m_undoCommand != nullptr || m_redoCommand != nullptr || m_currentCommand != nullptr;
}

bool CommandList::CanRedo()
{
	return m_redoStack.size() > 0 && !HasWork();
}

void CommandList::Redo()
{
	if (CanRedo())
	{
		m_redoCommand = std::move(m_redoStack.at(m_redoStack.size() - 1));
		m_redoStack.pop_back();
	}
}

void CommandList::Undo()
{
	if (CanUndo())
	{
		m_undoCommand = std::move(m_undoStack.at(m_undoStack.size() - 1));
		m_undoStack.pop_back();
	}
}

bool CommandList::CanUndo()
{
	return m_undoStack.size() > 0 && !HasWork();
}

void CommandList::ShowWindow()
{
	bool keepOpen = true;
	auto gui = Engine::GetSystem<Engine::DebugGuiSystem>("DebugGui");
	if (gui->BeginWindow(keepOpen, "Command List"))
	{
		if (gui->BeginListbox("Command Queue", {160,160}))
		{
			for (const auto& cmd : m_commandsToRun)
			{
				gui->Text(cmd->GetName());
			}
			gui->EndListbox();
		}
		gui->SameLine(0, 16.0f);
		if (gui->BeginListbox("Undo Stack", {160, 160}))
		{
			for (const auto& cmd : m_undoStack)
			{
				gui->Text(cmd->GetName());
			}
			gui->EndListbox();
		}
		gui->SameLine(0, 16.0f);
		if (gui->BeginListbox("Redo Stack", { 160, 160 }))
		{
			for (const auto& cmd : m_redoStack)
			{
				gui->Text(cmd->GetName());
			}
			gui->EndListbox();
		}
		if (m_currentCommand != nullptr)
		{
			gui->Text("Currently running: %s", m_currentCommand->GetName());
		}

		gui->EndWindow();
	}
}

void CommandList::Push(std::unique_ptr<Command>&& cmdPtr)
{
	m_commandsToRun.emplace_back(std::move(cmdPtr));
}

void CommandList::RunNext()
{
	SDE_PROF_EVENT();
	if (m_undoCommand != nullptr)
	{
		Command::Result undoResult = m_undoCommand->Undo();
		if (undoResult == Command::Result::Succeeded)
		{
			m_redoStack.push_back(std::move(m_undoCommand));
		}
		else if (undoResult == Command::Result::Failed)
		{
			m_undoCommand = nullptr;
		}
	}
	else if (m_redoCommand != nullptr)
	{
		Command::Result redoResult = m_redoCommand->Redo();
		if (redoResult == Command::Result::Succeeded)
		{
			m_undoStack.push_back(std::move(m_redoCommand));
		}
		else if (redoResult == Command::Result::Failed)
		{
			m_redoCommand = nullptr;
		}
	}
	else
	{
		if (m_commandsToRun.size() > 0 && m_currentCommand == nullptr)
		{
			m_currentCommand = std::move(m_commandsToRun[0]);
			m_commandsToRun.pop_front();
			m_redoStack.clear();
		}

		if (m_currentCommand != nullptr)
		{
			Command::Result result = m_currentCommand->Execute();
			if (result == Command::Result::Succeeded)
			{
				if (m_currentCommand->CanUndoRedo())
				{
					m_undoStack.push_back(std::move(m_currentCommand));
				}
				else
				{
					m_undoStack.clear();
				}
				m_currentCommand = nullptr;
			}
			else if (result == Command::Result::Failed)
			{
				SDE_LOG("A command failed to execute!\n\t(%s)", m_currentCommand->GetName());
				m_currentCommand = nullptr;
			}
		}
	}
}