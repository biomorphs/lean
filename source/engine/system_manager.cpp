#include "system_manager.h"
#include "system.h"
#include "core/string_hashing.h"
#include "core/log.h"
#include "core/profiler.h"
#include "core/timer.h"
#include <cassert>
#include <memory>

namespace Engine
{
	SystemManager::SystemManager()
	{
	}

	SystemManager::~SystemManager()
	{
	}

	SystemManager& SystemManager::GetInstance()
	{
		static SystemManager s_instance;
		return s_instance;
	}

	static SystemManager* SetInstance()
	{

	}

	void SystemManager::RegisterSystem(const char* systemName, System* theSystem)
	{
		assert(theSystem);
		uint32_t nameHash = Core::StringHashing::GetHash(systemName);
		assert(m_systemMap.find(nameHash) == m_systemMap.end());
		m_systems.push_back({ systemName, theSystem });
		m_systemMap.insert(SystemPair(nameHash, theSystem));
	}

	System* SystemManager::GetSystem(const char* systemName)
	{
		SystemMap::iterator it = m_systemMap.find(Core::StringHashing::GetHash(systemName));
		if (it != m_systemMap.end())
		{
			return it->second;
		}

		return nullptr;
	}

	bool SystemManager::Initialise()
	{
		SDE_PROF_FRAME("Main Thread");
		SDE_PROF_EVENT();
		{
			SDE_PROF_EVENT("PreInit");
			for (const auto it : m_systems)
			{
				if (!std::get<1>(it)->PreInit())
				{
					return false;
				}
			}
		}
		{
			SDE_PROF_EVENT("Initialise");
			for (const auto it : m_systems)
			{
				if (!std::get<1>(it)->Initialise())
				{
					return false;
				}
			}
		}
		{
			SDE_PROF_EVENT("PostInit");
			for (const auto it : m_systems)
			{
				if (!std::get<1>(it)->PostInit())
				{
					return false;
				}
			}
		}

		return true;
	}
	
	bool SystemManager::Tick(float timeDelta)
	{
		SDE_PROF_FRAME("Main Thread");
		SDE_PROF_EVENT();
		bool keepRunning = true;
		Core::Timer t;
		for (const auto it : m_systems)
		{
			auto startTime = t.GetSeconds();
			keepRunning &= std::get<1>(it)->Tick(timeDelta);
			auto endTime = t.GetSeconds();
			m_lastUpdateTime[std::get<0>(it)] = endTime - startTime;
		}
		return keepRunning;
	}
	
	void SystemManager::Shutdown()
	{
		SDE_PROF_FRAME("Main Thread");
		SDE_PROF_EVENT();
		for (const auto it : m_systems)
		{
			std::get<1>(it)->PreShutdown();
		}
		for (const auto it : m_systems)
		{
			std::get<1>(it)->Shutdown();
		}
		for (const auto it : m_systems)
		{
			std::get<1>(it)->PostShutdown();
		}
		for (const auto it : m_systems)
		{
			delete std::get<1>(it);
		}
		m_systems.clear();
		m_systemMap.clear();
	}
}