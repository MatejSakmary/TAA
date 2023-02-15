#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../shared/shared.inl"

inline auto get_taa_pass_pipeline(const RendererContext & context) -> daxa::ComputePipelineCompileInfo 
{
    daxa::ShaderCompileOptions compile_options;
    if(context.conditionals.color_clamp) { compile_options.defines.push_back({"COLOR_CLAMP", ""}); }
    if(context.conditionals.velocity_rejection) { compile_options.defines.push_back({"REJECT_VELOCITY", ""}); }
    if(context.conditionals.velocity_reproject) { compile_options.defines.push_back({"REPROJECT_VELOCITY", ""}); }
    if(context.conditionals.nearest_depth) { compile_options.defines.push_back({"NEAREST_DEPTH", ""}); }
    if(context.conditionals.accumulate) { compile_options.defines.push_back({"ACCUMULATE", ""}); }
    return {
        .shader_info = { 
            .source = daxa::ShaderFile{"taa.glsl"},
            .compile_options = compile_options
        },
        .push_constant_size = sizeof(TAAPC),
        .debug_name = "taa pass pipeline"
    };
}

inline void task_taa_pass(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY,
            },
        },
        .used_images = 
        {
            {
                context.main_task_list.images.t_accumulation_image,
                daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_offscreen_image,
                daxa::TaskImageAccess::COMPUTE_SHADER_READ_WRITE,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_offscreen_copy_image,
                daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_depth_image,
                daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{.image_aspect = daxa::ImageAspectFlagBits::DEPTH}
            },
            {
                context.main_task_list.images.t_velocity_image,
                daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_prev_velocity_image,
                daxa::TaskImageAccess::COMPUTE_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto dimensions = context.swapchain.get_surface_extent();

            auto accumulation_image = runtime.get_images(context.main_task_list.images.t_accumulation_image);
            auto offscreen_image = runtime.get_images(context.main_task_list.images.t_offscreen_image);
            auto offscreen_copy_image = runtime.get_images(context.main_task_list.images.t_offscreen_copy_image);
            auto depth_image = runtime.get_images(context.main_task_list.images.t_depth_image);
            auto velocity_image = runtime.get_images(context.main_task_list.images.t_velocity_image);
            auto prev_velocity_image = runtime.get_images(context.main_task_list.images.t_prev_velocity_image);
            auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);

            cmd_list.set_pipeline(*context.pipelines.p_taa_pass);
            cmd_list.push_constant(TAAPC{
                .transforms = context.device.get_device_address(transforms_buffer[0]),
                .depth_image          = depth_image[0].default_view(),
                .offscreen_image      = offscreen_image[0].default_view(),
                .offscreen_copy_image = offscreen_copy_image[0].default_view(),
                .velocity_image       = velocity_image[0].default_view(),
                .prev_velocity_image  = prev_velocity_image[0].default_view(),
                .accumulation_image   = accumulation_image[0].default_view(),
                .nearest_sampler      = context.nearest_sampler,
                .swapchain_dimensions = {dimensions.x, dimensions.y},
                .first_frame = context.conditionals.clear_accumulation ? 1u : 0u
            });
            cmd_list.dispatch(((dimensions.x + 7) / 8), ((dimensions.y + 3) / 4));
            if(context.conditionals.clear_accumulation == true) { context.conditionals.clear_accumulation = false; }
        },
        .debug_name = "task taa pass"
    });
}