#pragma once

#include <daxa/daxa.inl>

struct TransformData
{
    daxa_f32mat4x4 m_proj_view;
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

struct DrawScenePC
{
    daxa_BufferPtr(TransformData) transforms;
    daxa_BufferPtr(SceneGeometryVertices) vertices;
    daxa_u32 index_offset;
    daxa_f32mat4x4 m_model;
};