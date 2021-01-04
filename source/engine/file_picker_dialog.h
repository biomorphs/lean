#pragma once
#include <string>

namespace Engine
{
	std::string ShowFilePicker(std::string windowTitle, std::string rootDirectory = "", const char* filter = "All\0*.*\0", bool createNewFile = false);
}