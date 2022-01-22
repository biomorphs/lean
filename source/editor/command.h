#pragma once

class Command
{
public:
	virtual ~Command() {}
	enum class Result
	{
		Succeeded,	// the command finished executing and succeeded
		Failed,		// the command finished executing and failed 
		Waiting,	// the command needs to wait for something, try again later
	};
	virtual const char* GetName() = 0;
	virtual bool CanUndo() = 0;
	virtual Result Execute() = 0;
	virtual Result Undo() { return Result::Failed; }

private:
};