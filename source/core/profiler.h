#pragma once

//#define SDE_USE_OPTICK

// Assume any macros in here are active for the current scope
#ifdef SDE_USE_OPTICK
	#include <optick.h>
	#define SDE_PROF_FRAME(...)		OPTICK_FRAME(__VA_ARGS__)
	#define SDE_PROF_EVENT(...)		OPTICK_EVENT(__VA_ARGS__)
	#define SDE_PROF_EVENT_DYN(str) OPTICK_EVENT_DYNAMIC(str)
	#define SDE_PROF_STALL(...)		OPTICK_CATEGORY(__VA_ARGS__, Optick::Category::Wait)
	#define SDE_PROF_THREAD(name)	OPTICK_THREAD(name)
#else
	#define SDE_PROF_FRAME(...)
	#define SDE_PROF_EVENT(...)
	#define SDE_PROF_STALL(...)
	#define SDE_PROF_PUSH(name)	
	#define SDE_PROF_POP()
	#define SDE_PROF_THREAD(name)
#endif