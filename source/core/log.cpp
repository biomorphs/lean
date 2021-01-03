#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma warning(push)	
#pragma warning(disable: 4996)		// for strncat

namespace LogInternals
{
	// Log to the debug output window and the console
	void LogImpl(const char* txt, ...)
	{
		const size_t stackBufferSize = 1024;
		char stackOutputBuffer[stackBufferSize] = { '\0' };

		va_list vl;
		va_start(vl, txt);
		size_t strLength = _vscprintf(txt, vl);
		if ((strLength + 3) < stackBufferSize)
		{
			vsprintf_s(stackOutputBuffer, txt, vl);
			strcat_s(stackOutputBuffer, "\r\n");
			OutputDebugString(stackOutputBuffer);
			printf_s("%s", stackOutputBuffer);
		}
		else
		{
			std::unique_ptr<char[]> strBuffer(new char[strLength + 3]);
			vsnprintf(strBuffer.get(), strLength, txt, vl);
			strncat(strBuffer.get(), "\r\n", strLength + 2);
			strBuffer[strLength + 2] = { '\0' };
			OutputDebugString(strBuffer.get());
			printf_s("%s", strBuffer.get());
		}
		va_end(vl);
		

		
	}
}

#pragma warning(pop)