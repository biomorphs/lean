#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <map>

namespace Engine
{
	class System;

	// This class handles ownership and updates of systems
	// It is the *only* singleton allowed!
	class SystemManager
	{
	public:
		~SystemManager();

		static SystemManager& GetInstance();

		virtual System* GetSystem(const char* systemName);
		void RegisterSystem(const char* systemName, System* theSystem);
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();

		using ProfilerData = std::map<std::string, double>;
		const ProfilerData& GetLastUpdateTimes() const { return m_lastUpdateTime; }

	private:
		SystemManager();

		typedef std::vector<std::tuple<std::string, System*>> SystemArray;
		typedef std::map<uint32_t, System*> SystemMap;
		typedef std::pair<uint32_t, System*> SystemPair;

		ProfilerData m_lastUpdateTime;
		SystemArray m_systems;
		SystemMap m_systemMap;
	};

	template<class T>
	inline T* GetSystem(const char* name)	// Helper for less typing!
	{
		return static_cast<T*>(SystemManager::GetInstance().GetSystem(name));
	};
}