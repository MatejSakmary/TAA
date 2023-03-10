#pragma once

#include <span>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/shared.inl"

inline auto get_draw_debug_lights_pipeline(const RendererContext & context) -> daxa::RasterPipelineCompileInfo 
{
    return {
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
                .format = context.offscreen_format,
            },
        },
        .depth_test = {
            .depth_attachment_format = daxa::Format::D32_SFLOAT,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .raster = {
            .primitive_topology = daxa::PrimitiveTopology::POINT_LIST,
            .primitive_restart_enable = false,
            .polygon_mode = daxa::PolygonMode::POINT
        },
        .push_constant_size = sizeof(DrawDebugLightsPC),
    };
}

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
                context.main_task_list.images.t_offscreen_image,
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
            auto backbuffer_image = runtime.get_images(context.main_task_list.images.t_offscreen_image);
            auto depth_image = runtime.get_images(context.main_task_list.images.t_depth_image);
            auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);
            auto lights_buffer = runtime.get_buffers(context.main_task_list.buffers.t_scene_lights);

            cmd_list.begin_renderpass({
                .color_attachments = 
                {{
                    .image_view = backbuffer_image[0].default_view(),
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