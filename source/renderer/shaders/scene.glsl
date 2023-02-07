#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawScenePC)
daxa_BufferPtr(SceneGeometryVertices) scene_vertices = daxa_push_constant.vertices;
daxa_BufferPtr(TransformData) camera_transforms = daxa_push_constant.transforms;

#if defined(_VERTEX)
// ===================== VERTEX SHADER ===============================
layout (location = 0) out f32vec3 normal_out;
layout (location = 1) out f32vec4 prev_pos;
layout (location = 2) out f32vec4 curr_pos;
void main()
{
    f32vec4 pre_trans_pos = f32vec4(deref(scene_vertices[gl_VertexIndex + daxa_push_constant.index_offset]).position, 1.0);

    f32mat4x4 m_curr_proj_view_model = deref(camera_transforms).m_proj_view * daxa_push_constant.m_model;
    f32mat4x4 m_prev_proj_view_model = deref(camera_transforms).m_prev_proj_view * daxa_push_constant.m_model;

    gl_Position = deref(camera_transforms).m_jitter * m_curr_proj_view_model * pre_trans_pos;

    normal_out = deref(scene_vertices[gl_VertexIndex + daxa_push_constant.index_offset]).normal;
    curr_pos = m_curr_proj_view_model * pre_trans_pos;
    prev_pos = m_prev_proj_view_model * pre_trans_pos;
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) in f32vec3 normal_in;
layout (location = 1) in f32vec4 prev_pos;
layout (location = 2) in f32vec4 curr_pos;

layout (location = 0) out f32vec4 out_color;
layout (location = 1) out f32vec4 out_velocity;
layout (location = 2) out f32vec4 out_color_copy;

void main()
{
    f32vec2 prev_pos_div = f32vec2((prev_pos.xy / prev_pos.w) * 0.5 + 0.5);
    f32vec2 curr_pos_div = f32vec2((curr_pos.xy / curr_pos.w) * 0.5 + 0.5);
    f32vec2 velocity = curr_pos_div - prev_pos_div;
    out_color = f32vec4((normal_in + 1.0) / 2.0, 1.0);
    out_color_copy = f32vec4((normal_in + 1.0) / 2.0, 1.0);
    out_velocity = f32vec4(velocity, 0.0, 1.0);
}
#endif