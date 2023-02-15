#pragma once

#include <daxa/daxa.inl>


struct TransformData
{
    daxa_f32mat4x4 m_prev_proj_view;
    daxa_f32mat4x4 m_inv_proj_view;
    daxa_f32mat4x4 m_proj_view;
    daxa_f32mat4x4 m_jitter;
};

struct SceneLights
{
    daxa_f32vec4 position;
};

struct SceneGeometryVertices
{
    daxa_f32vec3 position;
    daxa_f32vec3 normal;
};

struct SceneGeometryIndices
{
    daxa_u32 index;
};

DAXA_ENABLE_BUFFER_PTR(TransformData)
DAXA_ENABLE_BUFFER_PTR(SceneGeometryVertices)
DAXA_ENABLE_BUFFER_PTR(SceneGeometryIndices)
DAXA_ENABLE_BUFFER_PTR(SceneLights)

struct DrawScenePC
{
    daxa_BufferPtr(TransformData) transforms;
    daxa_BufferPtr(SceneGeometryVertices) vertices;
    daxa_u32 index_offset;
    daxa_f32mat4x4 m_model;
};

struct DrawDebugLightsPC
{
    daxa_BufferPtr(TransformData) transforms;
    daxa_BufferPtr(SceneLights) lights;
};

struct TonemapPC
{
    daxa_Image2Df32 offscreen_image;
    daxa_SamplerId linear_sampler;
};

struct TAAPC
{
    daxa_BufferPtr(TransformData) transforms;
    daxa_Image2Df32 depth_image;
    daxa_RWImage2Df32 offscreen_image;
    daxa_RWImage2Df32 offscreen_copy_image;
    daxa_RWImage2Df32 velocity_image;
    daxa_RWImage2Df32 prev_velocity_image;
    daxa_RWImage2Df32 accumulation_image;
    daxa_SamplerId nearest_sampler;
    daxa_u32vec2 swapchain_dimensions;
    daxa_u32 first_frame;
};