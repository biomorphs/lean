/*
SDLEngine
Matt Hoyle
*/
#pragma once

#include "core/log.h"
#include <cassert>
#include <SDL.h>
#include <GL/glew.h>

// Renderer utils (global error handling, etc)

inline const char* TranslateGLError(uint32_t errorCode)
{
	switch (errorCode)
	{
	case GL_INVALID_VALUE:
		return "Invalid value";
	case GL_INVALID_ENUM:
		return "Invalid enum";
	case GL_INVALID_OPERATION:
		return "Invalid operation";
	case GL_OUT_OF_MEMORY:
		return "Out of memory";
	default:
		return "No errors";
	}
	return nullptr;
}

#define SDE_RENDER_ASSERT(condition, ...)				\
	assert(condition);

#define SDE_RENDER_PROCESS_GL_ERRORS(...)	\
	{\
		auto error = glGetError();	\
		while (error  != GL_NO_ERROR)	\
		{\
			SDE_LOGC(Render, "%s returned %d (%s)", ##__VA_ARGS__, error, TranslateGLError(error)); \
			assert(false); \
			error = glGetError();	\
		}\
	}

// Returns false if any errors occured
#define SDE_RENDER_PROCESS_GL_ERRORS_RET(...)	\
	{\
		bool shouldReturn = false;	\
		auto error = glGetError();	\
		while (error  != GL_NO_ERROR)	\
		{\
			SDE_LOGC(Render, "%s returned %d (%s)", ##__VA_ARGS__, error, TranslateGLError(error)); \
			assert(false); \
			error = glGetError();	\
			shouldReturn = true;	\
		}\
		if ( shouldReturn )	\
		{\
			return false;\
		}\
	}