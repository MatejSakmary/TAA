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

    f32vec4[9] neighbors;
    f32[9] gauss_weights = f32[](
        1.0/16.0, 1.0/8.0, 1.0/16.0, 
        1.0/ 8.0, 1.0/4.0, 1.0/ 8.0,
        1.0/16.0, 1.0/8.0, 1.0/16.0
    );

    f32vec4 blurred_col = f32vec4(0.0);
    f32 closest_depth = 1.0;
    i32vec2 depth_thread_xy = thread_xy;

    f32vec4 min_color = f32vec4(10.0e5);
    f32vec4 max_color = f32vec4(-10.0e5); 
    for(i32 y = 1; y > -2; y--)
    {
        for(i32 x = 1; x > -2; x--)
        {
            i32 index = ((y + 1) * 3) + (x + 1);
            neighbors[index] = imageLoad(daxa_push_constant.offscreen_copy_image, thread_xy + i32vec2(x, y));
            f32vec2 uv_offset = f32vec2(x,y) / f32vec2(daxa_push_constant.swapchain_dimensions);
            f32 depth = texture(daxa_push_constant.depth_image, daxa_push_constant.nearest_sampler, in_uv + uv_offset).r;

            closest_depth = min(depth, closest_depth);
            depth_thread_xy = i32(closest_depth == depth) * (thread_xy + i32vec2(x, y)) + i32(closest_depth != depth) * depth_thread_xy;

            min_color = min(neighbors[index], min_color);
            max_color = max(neighbors[index], max_color);

            blurred_col += gauss_weights[index] * neighbors[index];
        } 
    }

f32vec4 offscreen_color = neighbors[5];
#if defined(NEAREST_DEPTH)
    f32vec2 velocity = imageLoad(daxa_push_constant.velocity_image, depth_thread_xy).rg;
#else
    f32vec2 velocity = imageLoad(daxa_push_constant.velocity_image, thread_xy).rg;
#endif

f32 accum_factor = max(0.1, f32(daxa_push_constant.first_frame));

#if defined(REPROJECT_VELOCITY)
    f32vec2 vel_shift_uv = in_uv + velocity;
    i32vec2 accum_xy = i32vec2(vel_shift_uv * daxa_push_constant.swapchain_dimensions);
    f32vec4 accumulation_color = imageLoad(daxa_push_constant.accumulation_image, accum_xy);

    if(vel_shift_uv.x < 0.0 || vel_shift_uv.x > 1.0 || vel_shift_uv.y < 0.0 || vel_shift_uv.y > 1.0)
    {
        accum_factor = 1.0;
    }
#else
    f32vec4 accumulation_color = imageLoad(daxa_push_constant.accumulation_image, thread_xy);
#endif

#if defined(COLOR_CLAMP)
    accumulation_color = clamp(accumulation_color, min_color, max_color);
#endif

#if defined(ACCUMULATE)
    f32vec4 out_color = offscreen_color * accum_factor + accumulation_color * (1 - accum_factor);
#else
    f32vec4 out_color = offscreen_color;
#endif

#if defined(REJECT_VELOCITY)
    f32vec2 prev_velocity = imageLoad(daxa_push_constant.prev_velocity_image, accum_xy).rg;
    f32 velocity_len = length(prev_velocity - velocity);
    f32 velocity_disocclusion = clamp((velocity_len - 0.001) * 10.0, 0.0, 1.0);
    out_color = mix(out_color, blurred_col, velocity_disocclusion);
#endif

    imageStore(daxa_push_constant.offscreen_image, thread_xy , out_color);
}