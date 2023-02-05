#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <shared/draw_scene_shared.inl>

DAXA_USE_PUSH_CONSTANT(DrawDebugLightsPC)
daxa_BufferPtr(SceneLights) scene_lights = daxa_push_constant.lights;
daxa_BufferPtr(TransformData) camera_transforms = daxa_push_constant.transforms;

#if defined(_VERTEX)
// ===================== VERTEX SHADER ===============================
void main()
{
    f32vec4 pre_trans_pos = f32vec4(deref(scene_lights[gl_VertexIndex]).position);

    gl_PointSize = 10.0;
    gl_Position = deref(camera_transforms).m_proj_view * pre_trans_pos;
}

#elif defined(_FRAGMENT)
// ===================== FRAGMENT SHADER ===============================
layout (location = 0) out f32vec4 out_color;

void main()
{
    out_color = f32vec4(1.0, 1.0, 1.0, 1.0);
}
#endif