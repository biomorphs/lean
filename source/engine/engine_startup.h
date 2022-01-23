#include <functional>
#pragma once

namespace Engine
{
	// This runs everything. Call it from main()!
	// Use systemCreationCb to create/register app-specific systems
	int Run(std::function<void()> systemCreationCb, int argc, char* args[]);
}