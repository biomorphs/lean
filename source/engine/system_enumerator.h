#pragma once

#include <string>

namespace Engine
{
	class System;

	// This class acts as an interface to find systems without exposing the manager
	class SystemEnumerator
	{
	public:
		virtual System* GetSystem(const char* systemName) = 0;
	};
}