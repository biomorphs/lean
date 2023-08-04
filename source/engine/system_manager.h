#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <functional>

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
		void RegisterTickFn(std::string_view name, std::function<bool()> fn);
		bool Initialise();
		bool Tick(float timeDelta);
		void Shutdown();
		std::function<bool()> GetTickFn(std::string name);

		using ProfilerData = std::map<std::string, double>;
		const ProfilerData& GetLastUpdateTimes() const { return m_lastUpdateTime; }

	private:
		SystemManager();

		typedef std::vector<std::tuple<std::string, System*>> SystemArray;
		typedef std::map<uint32_t, System*> SystemMap;
		typedef std::pair<uint32_t, System*> SystemPair;

		struct TickFnRecord {
			std::function<bool()> m_fn;
			std::string m_name;
		};
		using TickFns = std::unordered_map<uint32_t, TickFnRecord>;	// fn name hash -> record

		ProfilerData m_lastUpdateTime;
		SystemArray m_systems;
		SystemMap m_systemMap;
		TickFns m_tickFns;
	};

	template<class T>
	inline T* GetSystem(const char* name)	// Helper for less typing!
	{
		return static_cast<T*>(SystemManager::GetInstance().GetSystem(name));
	};
}