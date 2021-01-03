#pragma once

#include <string>
#include <vector>

// Helpers for loading raw data from external files
namespace Kernel
{
	namespace FileIO
	{
		bool LoadTextFromFile(const std::string& fileSrcPath, std::string& resultBuffer);
		bool LoadBinaryFile(const std::string& fileSrcPath, std::vector<uint8_t>& resultBuffer);
		bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& src);
	}
}