#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <shared/shared.inl>

DAXA_USE_PUSH_CONSTANT(TonemapPC)

#if defined(_VERTEX)

layout (location = 0) out f32vec2 out_uv;
vec2 positions[3] = vec2[](
    vec2( 1.0, -3.0),
    vec2( 1.0,  1.0),
    vec2(-3.0,  1.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    out_uv = (positions[gl_VertexIndex] + 1.0) * 0.5;
}

#elif defined(_FRAGMENT)

layout (location = 0) in f32vec2 in_uv;
layout (location = 0) out f32vec4 out_color;
void main()
{
    f32vec4 offscreen_color = texture(daxa_push_constant.offscreen_image, daxa_push_constant.linear_sampler, in_uv);
    out_color = offscreen_color;
}

#endif