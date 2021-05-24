#pragma once
#include "core/profiler.h"

namespace Engine
{
	class SystemManager;
	class System
	{
	public:
		virtual ~System() {}
		virtual bool PreInit(SystemManager&) { SDE_PROF_EVENT(); return true; }
		virtual bool Initialise() { SDE_PROF_EVENT(); return true; }
		virtual bool PostInit() { SDE_PROF_EVENT(); return true; }

		virtual bool Tick(float timeDelta) { SDE_PROF_EVENT(); return true; }

		virtual void PreShutdown() { SDE_PROF_EVENT(); }
		virtual void Shutdown() { SDE_PROF_EVENT(); }
		virtual void PostShutdown() { SDE_PROF_EVENT(); }
	};
}