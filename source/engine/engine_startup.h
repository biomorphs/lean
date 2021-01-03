#include <functional>
#pragma once

namespace Engine
{
	class System;
	class SystemRegister
	{
	public:
		virtual void Register(const char* name, System* theSystem) = 0;
	};

	// This runs everything. Call it from main()!
	int Run(std::function<void(SystemRegister&)> systemCreation, int argc, char* args[]);
}