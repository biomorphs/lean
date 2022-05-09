#pragma once

// ALWAYS use this to include glm stuff
// It enables various features required to get decent perf in debug

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_EXPLICIT_CTOR					//disable implicit conversions between int and float types
#define GLM_FORCE_INLINE						// always inline
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES		// don't pack types, required for simd
#define GLM_FORCE_AVX
#ifndef NDEBUG
#define I_DISABLED_DEBUG
#define NDEBUG
#endif

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/closest_point.hpp>

#ifdef I_DISABLED_DEBUG
#undef NDEBUG
#endif