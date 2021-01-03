#pragma once

#include "system_enumerator.h"
#include <vector>
#include <map>

namespace Engine
{
	class System;

	// This class handles ownership and updates of systems
	class SystemManager : public SystemEnumerator
	{
	public:
		SystemManager();
		~SystemManager();

		// SystemEnumerator
		virtual System* GetSystem(const char* systemName);

		void RegisterSystem(const char* systemName, System* theSystem);

		bool Initialise();
		bool Tick();
		void Shutdown();

	private:
		typedef std::vector<System*> SystemArray;
		typedef std::map<uint32_t, System*> SystemMap;
		typedef std::pair<uint32_t, System*> SystemPair;

		SystemArray m_systems;
		SystemMap m_systemMap;
	};
}