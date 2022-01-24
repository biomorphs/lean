#pragma once

#include "core/log.h"
#include <cassert>
#include <SDL.h>
#include <GL/glew.h>

#define SDE_RENDER_ASSERT(condition, ...)				\
	assert(condition);
