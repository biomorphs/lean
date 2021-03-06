#include "script_system.h"
#include "core/file_io.h"
#include "core/profiler.h"
#include <sol.hpp>

namespace Engine
{
	ScriptSystem::ScriptSystem()
	{
	}

	ScriptSystem::~ScriptSystem()
	{
	}

	void ScriptSystem::OpenDefaultLibraries(sol::state& state)
	{
		SDE_PROF_EVENT();
		state.open_libraries(sol::lib::base, 
			sol::lib::math, 
			sol::lib::package,
			sol::lib::os,
			sol::lib::coroutine,
			sol::lib::bit32,
			sol::lib::io,
			sol::lib::debug,
			sol::lib::table, 
			sol::lib::string);
	}

	template<>
	bool ScriptSystem::RunScriptFromFile(const char* filename, std::string& errorText, sol::table& result) noexcept
	{
		try
		{
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "ScriptSystem::RunScriptFromFile(\"%s\")", filename);
			SDE_PROF_EVENT_DYN(debugName);

			std::string scriptText;
			if (Core::LoadTextFromFile(filename, scriptText))
			{
				result = m_globalState->script(scriptText.data());
			}

			return true;
		}
		catch (const sol::error& err)
		{
			errorText = err.what();
			return false;
		}
	}

	bool ScriptSystem::RunScriptFromFile(const char* filename, std::string& errorText) noexcept
	{
		try
		{
			char debugName[1024] = { '\0' };
			sprintf_s(debugName, "ScriptSystem::RunScriptFromFile(\"%s\")", filename);
			SDE_PROF_EVENT_DYN(debugName);

			std::string scriptText;
			if (Core::LoadTextFromFile(filename, scriptText))
			{
				m_globalState->script(scriptText.data());
			}

			return true;
		}
		catch (const sol::error& err)
		{
			errorText = err.what();
			return false;
		}
	}

	bool ScriptSystem::RunScript(const char* scriptSource, std::string& errorText) noexcept
	{
		try 
		{
			m_globalState->script(scriptSource);
			return true;
		}
		catch (const sol::error& err) 
		{
			errorText = err.what();
			return false;
		}
	}

	bool ScriptSystem::PreInit(SystemEnumerator& systemEnumerator)
	{
		SDE_PROF_EVENT();

		m_globalState = std::make_unique<sol::state>();
		OpenDefaultLibraries(*m_globalState);
		
		return true;
	}

	bool ScriptSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		{
			SDE_PROF_EVENT("CollectGarbage");
			m_globalState->collect_garbage();
		}
		return true;
	}

	void ScriptSystem::PostShutdown()
	{
		SDE_PROF_EVENT();

		m_globalState = nullptr;
	}
}