#pragma once

#include "types.hpp"
#include <daxa/types.hpp>

inline auto daxa_vec3_from_glm(const f32vec3 & vec) -> daxa::f32vec3 
{
    return {vec.x, vec.y, vec.z};
}

inline auto daxa_vec4_from_glm(const f32vec4 & vec) -> daxa::f32vec4 
{
    return {vec.x, vec.y, vec.z, vec.w};
}

#ifdef LOG_DEBUG
#include <iostream>
#define DEBUG_OUT(x) (std::cout << x << std::endl)
#define DEBUG_VAR_OUT(x) (std::cout << #x << ": " << (x) << std::endl)
#else
#define DEBUG_OUT(x)
#define DEBUG_VAR_OUT(x)
#endif