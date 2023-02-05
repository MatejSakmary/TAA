#pragma once

#include <daxa/daxa.inl>

struct TransformData
{
    daxa_f32mat4x4 m_proj_view;
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