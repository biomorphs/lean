#pragma once

#include "editor/command.h"

template<class ValueType>
class EditorSetValueCommand : public Command
{
public:
	using SetValueFn = std::function<void(ValueType)>;
	EditorSetValueCommand() = delete;
	EditorSetValueCommand(const char* name, ValueType currentValue, ValueType newValue, SetValueFn setFn)
		: m_currentValue(currentValue)
		, m_newValue(newValue)
		, m_setFn(setFn)
	{
		m_name = std::string("Set Value ") + name;
	}
	virtual ~EditorSetValueCommand() { }

	virtual const char* GetName() { return m_name.c_str(); }
	virtual Result Execute()
	{
		m_setFn(m_newValue);
		return Result::Succeeded;
	}
	virtual bool CanUndoRedo() { return true; }
	virtual Result Undo()
	{
		m_setFn(m_currentValue);
		return Result::Succeeded;
	}
	virtual Result Redo()
	{
		return Execute();
	}

private:
	std::string m_name;
	ValueType m_currentValue;
	ValueType m_newValue;
	SetValueFn m_setFn;
};