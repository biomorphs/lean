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

		fileStream.seekg(0, std::ios::end);
		const size_t fileSize = fileStream.tellg();

		std::unique_ptr<char[]> buf(new char[fileSize]);
		std::memset(buf.get(), 0, fileSize);
		fileStream.seekg(0, std::ios::beg);
		fileStream.read(buf.get(), fileSize);
		resultBuffer = buf.get();		

		return true;
	}

	bool LoadBinaryFile(const std::string& fileSrcPath, std::vector<uint8_t>& resultBuffer)
	{
		std::ifstream fileStream(fileSrcPath.c_str(), std::ios::binary | std::ios::in);
		if (!fileStream.is_open())
		{
			return false;
		}

		fileStream.seekg(0, std::ios::end);
		int fileSize = fileStream.tellg();
		fileStream.seekg(0, std::ios::beg);

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