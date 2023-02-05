#pragma once

#include <span>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/draw_scene_shared.inl"

inline static const daxa::RasterPipelineCompileInfo DRAW_DEBUG_LIGTS_RASTER_PIPE_INFO 
{
    .vertex_shader_info = {
        .source = daxa::ShaderFile{"debug_lights.glsl"},
        .compile_options = {
            .defines = {{"_VERTEX", ""}},
        },
    },
    .fragment_shader_info = {
        .source = daxa::ShaderFile{"debug_lights.glsl"},
        .compile_options = {
            .defines = {{"_FRAGMENT", ""}},
        },
    },
    .color_attachments = {
        daxa::RenderAttachment{
            .format = daxa::Format::B8G8R8A8_SRGB,
        },
    },
    .depth_test = {
        .depth_attachment_format = daxa::Format::D32_SFLOAT,
        .enable_depth_test = true,
        .enable_depth_write = true,
        // .depth_test_compare_op = daxa::CompareOp::GREATER_OR_EQUAL,
        // .min_depth_bounds = 1.0f,
        // .max_depth_bounds = 0.0f,
    },
    .raster = {
        .primitive_topology = daxa::PrimitiveTopology::POINT_LIST,
        .primitive_restart_enable = false,
        .polygon_mode = daxa::PolygonMode::POINT
    },
    .push_constant_size = sizeof(DrawDebugLightsPC),
};

inline void task_draw_debug_ligts(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_scene_lights,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            }
        },
        .used_images =
        {
            { 
                context.main_task_list.images.t_swapchain_image,
                daxa::TaskImageAccess::SHADER_WRITE_ONLY,
                daxa::ImageMipArraySlice{} 
            },
            { 
                context.main_task_list.images.t_depth_image,
                daxa::TaskImageAccess::DEPTH_ATTACHMENT,
                daxa::ImageMipArraySlice{.image_aspect = daxa::ImageAspectFlagBits::DEPTH} 
            }
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            if(context.buffers.scene_lights.cpu_buffer.empty()) { return; }
            auto cmd_list = runtime.get_command_list();
            auto dimensions = context.swapchain.get_surface_extent();
            auto swapchain_image = runtime.get_images(context.main_task_list.images.t_swapchain_image);
            auto depth_image = runtime.get_images(context.main_task_list.images.t_depth_image);
            auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);
            auto lights_buffer = runtime.get_buffers(context.main_task_list.buffers.t_scene_lights);

            cmd_list.begin_renderpass({
                .color_attachments = 
                {{
                    .image_view = swapchain_image[0].default_view(),
                    .load_op = daxa::AttachmentLoadOp::LOAD,
                }},
                .depth_attachment = 
                {{
                    .image_view = depth_image[0].default_view(),
                    .layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
                    .load_op = daxa::AttachmentLoadOp::LOAD,
                    .store_op = daxa::AttachmentStoreOp::STORE,
                }},
                .render_area = {.x = 0, .y = 0, .width = dimensions.x , .height = dimensions.y}
            });
            cmd_list.set_pipeline(*context.pipelines.p_draw_debug_lights);
            cmd_list.push_constant(DrawDebugLightsPC{
                .transforms = context.device.get_device_address(transforms_buffer[0]),
                .lights = context.device.get_device_address(lights_buffer[0])
            });
            cmd_list.draw({ .vertex_count = static_cast<u32>(context.buffers.scene_lights.cpu_buffer.size()) });
            cmd_list.end_renderpass();
        },
        .debug_name = "draw debug lights"
    });
}