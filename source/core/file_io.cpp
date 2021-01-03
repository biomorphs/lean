#include "file_io.h"
#include <cassert>
#include <fstream>

namespace Kernel
{
	namespace FileIO
	{
		bool LoadTextFromFile(const std::string& fileSrcPath, std::string& resultBuffer)
		{
			resultBuffer.clear();
			std::ifstream fileStream(fileSrcPath.c_str(), std::ios::in);
			if (!fileStream.is_open())
			{
				return false;
			}

			char oneLine[512] = { '\0' };
			while (!fileStream.eof())
			{
				fileStream.getline(oneLine, 512);
				resultBuffer += oneLine;
				resultBuffer += "\n";
			}
			fileStream.close();
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
			std::streamoff fileSize = fileStream.tellg();
			fileStream.seekg(0, std::ios::beg);

			resultBuffer.resize(static_cast<uint32_t>(fileSize));
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
}