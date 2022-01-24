#include "file_picker_dialog.h"
#include "core/profiler.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <algorithm>

namespace Engine
{
	std::string ShowFilePicker(std::string windowTitle, std::string rootDirectory, const char* filter, bool createNewFile)
	{
		SDE_PROF_EVENT();

		OPENFILENAMEA ofa;
		memset(&ofa, 0, sizeof(ofa));
		ofa.lStructSize = sizeof(ofa);

		// win32 is a pain and uses lists of null-terminated strings for filters
		// we need to double-terminate a string or it will read random memory
		ofa.lpstrFilter = filter;
		ofa.nFilterIndex = 1;

		// make sure we have a nice big buffer for reading the filename
		std::string filenameBuffer = rootDirectory;
		filenameBuffer.append(1024, '\0');

		ofa.lpstrFile = filenameBuffer.data();
		ofa.nMaxFile = filenameBuffer.size() - 1;
		ofa.lpstrTitle = windowTitle.c_str();
		ofa.nMaxFileTitle = windowTitle.length();
		if (createNewFile)
		{
			ofa.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
		}
		else
		{
			ofa.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		}

		bool fileSelected = GetOpenFileNameA(&ofa);
		if (fileSelected)
		{
			return ofa.lpstrFile;
		}
		else
		{
			return "";
		}
	}
}

#endif