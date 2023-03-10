#pragma once

#include <span>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/shared.inl"

inline auto get_draw_scene_pipeline(const RendererContext & context) -> daxa::RasterPipelineCompileInfo
{
    daxa::ShaderCompileOptions compile_options;
    compile_options.defines.push_back({"_VERTEX", ""});
    if(context.conditionals.jitter_camera) { compile_options.defines.push_back({"JITTER_CAMERA", ""}); }

    return {
        .vertex_shader_info = {
            .source = daxa::ShaderFile{"scene.glsl"},
            .compile_options = compile_options
            },
        .fragment_shader_info = {
            .source = daxa::ShaderFile{"scene.glsl"},
            .compile_options = {
                .defines = {{"_FRAGMENT", ""}},
            },
        },
        .color_attachments = {
            daxa::RenderAttachment{ .format = context.offscreen_format, }, // Offscreen image
            daxa::RenderAttachment{ .format = context.velocity_format,  }, // Velocity image
            daxa::RenderAttachment{ .format = context.offscreen_format, }, // Offscreen copy image
        },
        .depth_test = {
            .depth_attachment_format = daxa::Format::D32_SFLOAT,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .raster = {
            .primitive_topology = daxa::PrimitiveTopology::TRIANGLE_LIST,
            .primitive_restart_enable = false,
            .polygon_mode = daxa::PolygonMode::FILL
        },
        .push_constant_size = sizeof(DrawScenePC),
    };
}

inline void task_draw_scene(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_scene_vertices,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_scene_indices,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::SHADER_READ_ONLY,
            },
        },
        .used_images =
        {
            { 
                context.main_task_list.images.t_offscreen_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_WRITE_ONLY,
                daxa::ImageMipArraySlice{} 
            },
            { 
                context.main_task_list.images.t_velocity_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_WRITE_ONLY,
                daxa::ImageMipArraySlice{} 
            },
            { 
                context.main_task_list.images.t_offscreen_copy_image,
                daxa::TaskImageAccess::FRAGMENT_SHADER_WRITE_ONLY,
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
            auto cmd_list = runtime.get_command_list();
            auto dimensions = context.swapchain.get_surface_extent();

            auto offscreen_image = runtime.get_images(context.main_task_list.images.t_offscreen_image);
            auto offscreen_copy_image = runtime.get_images(context.main_task_list.images.t_offscreen_copy_image);
            auto velocity_image = runtime.get_images(context.main_task_list.images.t_velocity_image);
            auto depth_image = runtime.get_images(context.main_task_list.images.t_depth_image);

            auto index_buffer = runtime.get_buffers(context.main_task_list.buffers.t_scene_indices);
            auto vertex_buffer = runtime.get_buffers(context.main_task_list.buffers.t_scene_vertices);
            auto transforms_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);

            cmd_list.reset_timestamps({ 
                .query_pool = context.timestamps,
                .start_index = 0,
                .count = context.timestamps.info().query_count
            });

            cmd_list.begin_renderpass({
                .color_attachments = 
                { 
                    {
                        .image_view = offscreen_image[0].default_view(),
                        .load_op = daxa::AttachmentLoadOp::CLEAR,
                        .clear_value = std::array<f32, 4>{0.02, 0.02, 0.02, 1.0},
                    },
                    {
                        .image_view = velocity_image[0].default_view(),
                        .load_op = daxa::AttachmentLoadOp::CLEAR,
                        .clear_value = std::array<f32, 4>{0.0, 0.0, 0.0, 1.0},
                    },
                    {
                        .image_view = offscreen_copy_image[0].default_view(),
                        .load_op = daxa::AttachmentLoadOp::CLEAR,
                        .clear_value = std::array<f32, 4>{0.02, 0.02, 0.02, 1.0},
                    }
                },
                .depth_attachment = 
                {{
                    .image_view = depth_image[0].default_view(),
                    .layout = daxa::ImageLayout::ATTACHMENT_OPTIMAL,
                    .load_op = daxa::AttachmentLoadOp::CLEAR,
                    .store_op = daxa::AttachmentStoreOp::STORE,
                    .clear_value = daxa::ClearValue{daxa::DepthValue{1.0f, 0}},
                }},
                .render_area = {.x = 0, .y = 0, .width = dimensions.x , .height = dimensions.y}
            });

            cmd_list.set_pipeline(*context.pipelines.p_draw_scene);

            cmd_list.write_timestamp({ 
                .query_pool = context.timestamps,
                .pipeline_stage = daxa::PipelineStageFlagBits::BOTTOM_OF_PIPE,
                .query_index = 0
            });
            // NOTE(msakmary) I can't put const auto & object here since than the span constructor complains
            // and I don't know how to fig this
            for(auto & object : context.render_info.objects)
            {
                for(const auto & mesh : object.meshes)
                {
                    cmd_list.push_constant(DrawScenePC{
                        .transforms = context.device.get_device_address(transforms_buffer[0]),
                        .vertices = context.device.get_device_address(vertex_buffer[0]),
                        .index_offset = mesh.index_offset,
                        .m_model = daxa::math_operators::mat_from_span<daxa::f32, 4, 4>(
                            std::span<daxa::f32, 4 * 4>{glm::value_ptr(object.model_transform), 4 * 4})
                    });
                    cmd_list.set_index_buffer(index_buffer[0], sizeof(u32) * (mesh.index_buffer_offset) , sizeof(u32));
                    cmd_list.draw_indexed({ .index_count = mesh.index_count});
                }
            }

            cmd_list.write_timestamp({ 
                .query_pool = context.timestamps,
                .pipeline_stage = daxa::PipelineStageFlagBits::BOTTOM_OF_PIPE,
                .query_index = 1
            });
            cmd_list.end_renderpass();
        },
        .debug_name = "draw scene",
    });
}