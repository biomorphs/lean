#pragma once 
#include <deque>
#include <memory>

class Command;

class CommandList
{
public:
	void ShowWindow();
	
	void Push(std::unique_ptr<Command>&& cmdPtr);
	bool HasWork();
	void RunNext();

	bool CanRedo();
	void Redo();
	bool CanUndo();
	void Undo();
	
private:
	std::unique_ptr<Command> m_undoCommand;
	std::unique_ptr<Command> m_currentCommand;
	std::unique_ptr<Command> m_redoCommand;
	std::deque<std::unique_ptr<Command>> m_redoStack;
	std::deque<std::unique_ptr<Command>> m_commandsToRun;
	std::deque<std::unique_ptr<Command>> m_undoStack;
};