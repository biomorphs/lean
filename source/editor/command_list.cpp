#include "command_list.h"
#include "command.h"
#include "core/log.h"

void CommandList::Push(std::unique_ptr<Command>&& cmdPtr)
{
	m_commandsToRun.emplace_back(std::move(cmdPtr));
}

void CommandList::RunNext()
{
	if (m_commandsToRun.size() > 0 && m_currentCommand == nullptr)
	{
		m_currentCommand = std::move(m_commandsToRun[0]);
		m_commandsToRun.pop_front();
	}

	if (m_currentCommand != nullptr)
	{
		Command::Result result = m_currentCommand->Execute();
		if (result == Command::Result::Succeeded)
		{
			if (m_currentCommand->CanUndo())
			{
				m_undoStack.push_back(std::move(m_currentCommand));
			}
			else
			{
				m_currentCommand = nullptr;
				m_undoStack.clear();
			}
		}
		else if (result == Command::Result::Failed)
		{
			SDE_LOG("A command failed to execute!\n\t(%s)", m_currentCommand->GetName());
			m_currentCommand = nullptr;
		}
	}
}