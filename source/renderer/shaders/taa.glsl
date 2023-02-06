#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#include <shared/shared.inl>

DAXA_USE_PUSH_CONSTANT(TAAPC)
layout (local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
daxa_BufferPtr(TransformData) camera_transforms = daxa_push_constant.transforms;

void main()
{
    if(gl_GlobalInvocationID.x >= daxa_push_constant.swapchain_dimensions.x ||
       gl_GlobalInvocationID.y >= daxa_push_constant.swapchain_dimensions.y)
    {
        return;
    }

    i32vec2 thread_xy = i32vec2(gl_GlobalInvocationID.xy);
    f32vec2 in_uv = f32vec2(thread_xy) / f32vec2(daxa_push_constant.swapchain_dimensions - u32vec2(1));
    f32vec4 offscreen_color = imageLoad(daxa_push_constant.offscreen_image, thread_xy);

    f32vec2 velocity = imageLoad(daxa_push_constant.velocity_image, thread_xy).rg;
    f32vec2 vel_shift_uv = in_uv + velocity;
    i32vec2 accum_xy = i32vec2(vel_shift_uv * daxa_push_constant.swapchain_dimensions);
    f32vec4 accumulation_color = imageLoad(daxa_push_constant.accumulation_image, accum_xy);


    f32 accum_factor = max(0.1, f32(daxa_push_constant.first_frame));
    if(vel_shift_uv.x < 0.0 || vel_shift_uv.x > 1.0 || vel_shift_uv.y < 0.0 || vel_shift_uv.y > 1.0)
    {
        accum_factor = 1.0;
    }
    // accum_factor = 1.0;
    f32vec4 out_color = offscreen_color * accum_factor + accumulation_color * (1 - accum_factor);
    // imageStore(daxa_push_constant.offscreen_image, i32vec2(gl_GlobalInvocationID.xy) , f32vec4( proj_uv.xy, 0.0, 1.0));
    // imageStore(daxa_push_constant.offscreen_image, i32vec2(gl_GlobalInvocationID.xy) , f32vec4(world_pos, 1.0));
    imageStore(daxa_push_constant.offscreen_image, thread_xy , out_color);
}