#include "file_io.h"
#include "profiler.h"
#include <cassert>
#include <fstream>

namespace Core
{
	bool SaveTextToFile(const std::string& filePath, const std::string& src)
	{
		SDE_PROF_EVENT();

		std::ofstream fileStream(filePath.c_str(), std::ios::out);
		if (!fileStream.is_open())
		{
			return false;
		}
		fileStream.write((const char*)src.data(), src.size());
		fileStream.close();
		return true;
	}

	bool LoadTextFromFile(const std::string& fileSrcPath, std::string& resultBuffer)
	{
		SDE_PROF_EVENT();

		resultBuffer.clear();
		std::ifstream fileStream(fileSrcPath.c_str(), std::ios::in);
		if (!fileStream.is_open())
		{
			return false;
		}

		fileStream.seekg(0, fileStream.end);
		const size_t fileSize = fileStream.tellg();
		fileStream.seekg(0, fileStream.beg);

		resultBuffer.resize(fileSize);
		fileStream.read(resultBuffer.data(), fileSize);

		size_t actualSize = strlen(resultBuffer.data());
		resultBuffer.resize(actualSize);

		return true;
	}

	bool LoadBinaryFile(const std::string& fileSrcPath, std::vector<uint8_t>& resultBuffer)
	{
		std::ifstream fileStream(fileSrcPath.c_str(), std::ios::binary | std::ios::in);
		if (!fileStream.is_open())
		{
			return false;
		}

		fileStream.seekg(0, fileStream.end);
		const size_t fileSize = fileStream.tellg();
		fileStream.seekg(0, fileStream.beg);

		resultBuffer.resize(fileSize);
		fileStream.read(reinterpret_cast<char*>(resultBuffer.data()), fileSize);
		fileStream.close();

		return true;
	}

	bool SaveBinaryFile(const std::string& filePath, const std::vector<uint8_t>& src)
	{
		std::ofstream fileStream(filePath.c_str(), std::ios::binary | std::ios::out);
		if (!fileStream.is_open())
		{
			return false;
		}
		fileStream.write((const char*)src.data(), src.size());
		fileStream.close();
		return true;
	}
}