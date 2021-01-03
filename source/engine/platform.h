#pragma once

namespace Platform
{
	enum class InitResult
	{
		InitOK,
		InitFailed,
	};

	enum class ShutdownResult
	{
		ShutdownOK
	};

	int CPUCount();
	InitResult Initialise(int argc, char* argv[]);
	ShutdownResult Shutdown();
}