#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../shared/shared.inl"

static inline const daxa::RasterPipelineCompileInfo RASTER_TAA_PASS_PIPE_INFO
{
    .vertex_shader_info = {
        .source = daxa::ShaderFile{"taa.glsl"},
        .compile_options = {
            .defines = {{"_VERTEX", ""}},
        },
    },
    .fragment_shader_info = {
        .source = daxa::ShaderFile{"taa.glsl"},
        .compile_options = {
            .defines = {{"_FRAGMENT", ""}},
        },
    },
    .color_attachments = {
        daxa::RenderAttachment{
            .format = daxa::Format::B8G8R8A8_SRGB,
        }
    },
    .depth_test = {
        .enable_depth_test = false,
        .enable_depth_write = false,
    },
    .raster = {
        .primitive_topology = daxa::PrimitiveTopology::TRIANGLE_LIST,
        .primitive_restart_enable = false,
        .polygon_mode = daxa::PolygonMode::FILL
    },
    .push_constant_size = sizeof(DrawScenePC),
};

inline void task_taa_pass(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::FRAGMENT_SHADER_READ_ONLY,
            },
        },
        .used_images = 
        {
            {
                context.main_task_list.images.t_swapchain_image,
                daxa::TaskImageAccess::SHADER_READ_WRITE,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_resolve_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_backbuffer_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_depth_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{.image_aspect = daxa::ImageAspectFlagBits::DEPTH}
            },
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto dimensions = context.swapchain.get_surface_extent();

            auto swapchain_image = runtime.get_images(context.main_task_list.images.t_swapchain_image);
            auto resolve_image = runtime.get_images(context.main_task_list.images.t_resolve_image);
            auto backbuffer_image = runtime.get_images(context.main_task_list.images.t_backbuffer_image);
            auto depth_image = runtime.get_images(context.main_task_list.images.t_depth_image);
            auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);

            cmd_list.begin_renderpass({
                .color_attachments =
                {
                    {
                        .image_view = swapchain_image[0].default_view(),
                        .load_op = daxa::AttachmentLoadOp::CLEAR,
                        .clear_value = std::array<f32, 4>{0.00, 0.00, 0.00, 1.0},
                    }
                },
                .render_area = {.x = 0, .y = 0, .width = dimensions.x , .height = dimensions.y}
            });
            cmd_list.set_pipeline(*context.pipelines.p_taa_pass);
            cmd_list.push_constant(TAAPC{
                .transforms = context.device.get_device_address(transforms_buffer[0]),
                .depth_image = depth_image[0].default_view(),
                .resolve_image = resolve_image[0].default_view(),
                .backbuffer_image = backbuffer_image[0].default_view(),
                .swapchain_dimensions = daxa_u32vec2{dimensions.x, dimensions.y},
                .nearest_sampler = context.nearest_sampler,
                .first_frame = context.conditionals.clear_resolve ? 1u : 0u
            });
            cmd_list.draw({.vertex_count = 3});
            cmd_list.end_renderpass();
            if(context.conditionals.clear_resolve == true) { context.conditionals.clear_resolve = false; }
        },
        .debug_name = "task taa pass"
    });
}