#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../shared/shared.inl"

inline auto get_tonemap_pass_pipeline(const RendererContext & context) -> daxa::RasterPipelineCompileInfo 
{
    return {
        .vertex_shader_info = {
            .source = daxa::ShaderFile{"tonemap.glsl"},
            .compile_options = {
                .defines = {{"_VERTEX", ""}},
            },
        },
        .fragment_shader_info = {
            .source = daxa::ShaderFile{"tonemap.glsl"},
            .compile_options = {
                .defines = {{"_FRAGMENT", ""}},
            },
        },
        .color_attachments = {
            daxa::RenderAttachment{
                .format = context.swapchain.get_format(),
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
        .push_constant_size = sizeof(TonemapPC),
    };
}

inline void task_tonemap_pass(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers = { },
        .used_images = 
        {
            {
                context.main_task_list.images.t_offscreen_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_READ_ONLY,
                daxa::ImageMipArraySlice{}
            },
            {
                context.main_task_list.images.t_swapchain_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_WRITE_ONLY,
                daxa::ImageMipArraySlice{}
            },
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto dimensions = context.swapchain.get_surface_extent();

            auto swapchain_image = runtime.get_images(context.main_task_list.images.t_swapchain_image);
            auto offscreen_image = runtime.get_images(context.main_task_list.images.t_offscreen_image);

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
            cmd_list.set_pipeline(*context.pipelines.p_tonemap_pass);
            cmd_list.push_constant(TonemapPC{
                .offscreen_image = offscreen_image[0].default_view(),
                .linear_sampler = context.linear_sampler,
            });
            cmd_list.draw({.vertex_count = 3});
            cmd_list.end_renderpass();
        },
        .debug_name = "task tonemap pass"
    });
}