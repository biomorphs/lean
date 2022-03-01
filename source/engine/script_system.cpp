#include "script_system.h"
#include "system_manager.h"
#include "core/file_io.h"
#include "core/profiler.h"
#include "components/component_script.h"
#include "entity/entity_system.h"

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
			sol::lib::jit,
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
				result = m_globalState->script(scriptText.data(), [](lua_State*, sol::protected_function_result pfr) {
					sol::error err = pfr;
					SDE_LOG("Script error: %s", err.what());
					return pfr;	// something went wrong!
				});
				return result.valid();
			}

			return false;
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
				auto result = m_globalState->script(scriptText.data(), [](lua_State*, sol::protected_function_result pfr) {
					sol::error err = pfr;
					SDE_LOG("Script error: %s", err.what());
					return pfr;	// something went wrong!
				});
				return result.valid();
			}
			return false;
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
			auto result = m_globalState->script(scriptSource, [](lua_State*, sol::protected_function_result pfr) {
				sol::error err = pfr;
				SDE_LOG("Script error: %s", err.what());
				return pfr;	// something went wrong!
			});
			return result.valid();
		}
		catch (const sol::error& err) 
		{
			errorText = err.what();
			return false;
		}
	}

	bool ScriptSystem::PreInit()
	{
		SDE_PROF_EVENT();

		m_globalState = std::make_unique<sol::state>();
		OpenDefaultLibraries(*m_globalState);
		
		return true;
	}

	bool ScriptSystem::Initialise()
	{
		auto entities = Engine::GetSystem<EntitySystem>("Entities");
		entities->RegisterComponentType<Script>();
		entities->RegisterInspector<Script>(Script::MakeInspector());
		return true;
	}

	bool ScriptSystem::Tick(float timeDelta)
	{
		SDE_PROF_EVENT();

		{
			SDE_PROF_EVENT("CollectGarbage");
			m_globalState->collect_garbage();
		}

		{
			SDE_PROF_EVENT("RunScriptComponents");
			auto entities = Engine::GetSystem<EntitySystem>("Entities");
			auto world = entities->GetWorld();
			world->ForEachComponent<Script>([this](Script& s, EntityHandle e) {
				if (s.NeedsCompile())
				{
					if (s.GetFunctionText().length() == 0)
					{
						s.SetCompiledFunction({});
					}
					else
					{
						sol::protected_function theFn;
						try
						{
							theFn = m_globalState->script(s.GetFunctionText(), [](lua_State*, sol::protected_function_result pfr) {
								sol::error err = pfr;
								SDE_LOG("Script error: %s", err.what());
								return pfr;	// something went wrong!
							});
							if (theFn.valid())
							{
								s.SetCompiledFunction(theFn);
							}
						}
						catch (const sol::error& err)
						{
							std::string errorText = err.what();
							SDE_LOG("Script Error: %s", errorText.c_str());
							s.SetCompiledFunction({});
						}
					}
				}
				if (!s.NeedsCompile() && s.GetCompiledFunction().valid())
				{
					try
					{
						const auto& fn = s.GetCompiledFunction();
						sol::protected_function_result result = fn(e);
						if (!result.valid())
						{
							sol::error err = result;
							SDE_LOG("Script error: %s", err.what());
							SDE_LOG("Script Error!");
						}
					}
					catch (const sol::error& err)
					{
						std::string errorText = err.what();
						SDE_LOG("Script Error: %s", errorText.c_str());
					}
				}
			});
		}

		return true;
	}

	void ScriptSystem::PostShutdown()
	{
		SDE_PROF_EVENT();

		m_globalState = nullptr;
	}
}