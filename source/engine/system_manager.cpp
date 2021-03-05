#include "system_manager.h"
#include "system.h"
#include "core/string_hashing.h"
#include "core/log.h"
#include "core/profiler.h"
#include <cassert>

namespace Engine
{
	SystemManager::SystemManager()
	{
	}

	SystemManager::~SystemManager()
	{
	}

	void SystemManager::RegisterSystem(const char* systemName, System* theSystem)
	{
		assert(theSystem);
		uint32_t nameHash = Core::StringHashing::GetHash(systemName);
		assert(m_systemMap.find(nameHash) == m_systemMap.end());
		m_systems.push_back(theSystem);
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
				if (!it->PreInit(*this))
				{
					return false;
				}
			}
		}
		{
			SDE_PROF_EVENT("Initialise");
			for (const auto it : m_systems)
			{
				if (!it->Initialise())
				{
					return false;
				}
			}
		}
		{
			SDE_PROF_EVENT("PostInit");
			for (const auto it : m_systems)
			{
				if (!it->PostInit())
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
		for (const auto it : m_systems)
		{
			keepRunning &= it->Tick(timeDelta);
		}
		return keepRunning;
	}
	
	void SystemManager::Shutdown()
	{
		SDE_PROF_FRAME("Main Thread");
		SDE_PROF_EVENT();
		for (const auto it : m_systems)
		{
			it->PreShutdown();
		}
		for (const auto it : m_systems)
		{
			it->Shutdown();
		}
		for (const auto it : m_systems)
		{
			it->PostShutdown();
		}
		for (const auto it : m_systems)
		{
			delete it;
		}
		m_systems.clear();
		m_systemMap.clear();
	}
}