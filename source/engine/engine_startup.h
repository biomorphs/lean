#include <functional>
#pragma once

namespace Engine
{
	class FrameGraph;

	// This runs everything. Call it from main()!
	// Use systemCreationCb to create/register app-specific systems
	int Run(std::function<void()> systemCreationCb, std::function<void(FrameGraph&)> frameGraphBuildCb, int argc, char* args[]);
}