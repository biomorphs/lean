#pragma once 
#include <deque>
#include <memory>

class Command;

class CommandList
{
public:
	void Push(std::unique_ptr<Command>&& cmdPtr);
	void RunNext();
private:
	std::unique_ptr<Command> m_currentCommand;
	std::deque<std::unique_ptr<Command>> m_commandsToRun;
	std::deque<std::unique_ptr<Command>> m_undoStack;
};