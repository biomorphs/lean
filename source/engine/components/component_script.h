#pragma once

#include "entity/component.h"
#include "engine/script_system.h"

// contains a script that returns a function which is called every frame
// the function should have the parent entity handle as the only parameter
// do not do anything global in the script, just return the function!
// i.e.
// function MyScriptFn(entity)
//	do whatever
// end
// return MyScriptFn
// MyScriptFn will then be called once per frame

class Script
{
public:
	COMPONENT(Script);
	COMPONENT_INSPECTOR();

	std::string GetScriptFile() { return m_scriptFile; }
	void SetFile(const std::string& path) { m_scriptFile = path; m_needsCompile = true; }

	std::string GetFunctionText() { return m_scriptText; }
	void SetFunctionText(const std::string& text) { m_scriptText = text; m_needsCompile = true;	}

	bool NeedsCompile() const { return m_needsCompile; }
	void SetCompiledFunction(sol::protected_function fn) { m_compiledFn = fn; m_needsCompile = false; }
	const sol::protected_function& GetCompiledFunction() { return m_compiledFn; }

private:
	std::string m_scriptFile = "";
	std::string m_scriptText = "";
	bool m_needsCompile = false;
	sol::protected_function m_compiledFn;
};