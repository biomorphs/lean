#pragma once

#include "system.h"
#define SOL_CHECK_ARGUMENTS 1		// deprecated in sol 3 (time for upgrade?)
#define SOL_LUAJIT 1
#include <sol.hpp>
#include <string>
#include <memory>

namespace sol
{
	class state;
}

namespace Engine
{
	// Handles LUA via sol2. Exposes sol2 state directly since I cba wrapping it
	// (and the interface is nice enough)
	class ScriptSystem : public System
	{
	public:
		ScriptSystem();
		virtual ~ScriptSystem();

		bool PreInit(SystemEnumerator& systemEnumerator);
		bool Tick(float timeDelta);
		void PostShutdown();

		sol::state& Globals() { return *m_globalState; }
		
		template<class T>
		bool RunScriptFromFile(const char* filename, std::string& errorText, T& result) noexcept;

		bool RunScriptFromFile(const char* filename, std::string& errorText) noexcept;
		bool RunScript(const char* scriptSource, std::string& errorText) noexcept;

	private:
		void OpenDefaultLibraries(sol::state& state);
		std::unique_ptr<sol::state> m_globalState;
	};
}