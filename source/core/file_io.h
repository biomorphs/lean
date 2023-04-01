#pragma once

#include <string>
#include <vector>

// Helpers for loading raw data from external files
namespace Core
{
	bool LoadTextFromFile(const std::string& fileSrcPath, std::string& resultBuffer);
	bool SaveTextToFile(const std::string& filePath, const std::string& src);
	bool LoadBinaryFile(const std::string& fileSrcPath, std::vector<uint8_t>& resultBuffer);
	bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& src);

	// system path stuff
	void InitialisePaths();
	const std::string_view GetBasePath();
}