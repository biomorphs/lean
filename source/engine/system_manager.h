#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <map>

namespace Engine
{
	class System;

	// This class handles ownership and updates of systems
	class SystemManager
	{
	public:
		SystemManager();
		~SystemManager();

		virtual System* GetSystem(const char* systemName);
		void RegisterSystem(const char* systemName, System* theSystem);
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();

		using ProfilerData = std::map<std::string, double>;
		const ProfilerData& GetLastUpdateTimes() const { return m_lastUpdateTime; }

	private:
		typedef std::vector<std::tuple<std::string, System*>> SystemArray;
		typedef std::map<uint32_t, System*> SystemMap;
		typedef std::pair<uint32_t, System*> SystemPair;

		ProfilerData m_lastUpdateTime;
		SystemArray m_systems;
		SystemMap m_systemMap;
	};
}