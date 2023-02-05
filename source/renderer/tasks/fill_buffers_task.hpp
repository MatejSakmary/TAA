#pragma once

#include <cstring>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"


inline void task_fill_buffers(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers =
        {
            {
                context.main_task_list.buffers.t_transform_data,
                daxa::TaskBufferAccess::HOST_TRANSFER_WRITE,
            },
            {
                context.main_task_list.buffers.t_scene_vertices,
                daxa::TaskBufferAccess::HOST_TRANSFER_WRITE,
            },
            {
                context.main_task_list.buffers.t_scene_indices,
                daxa::TaskBufferAccess::HOST_TRANSFER_WRITE,
            }
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto transform_buffer = runtime.get_buffers(context.main_task_list.buffers.t_transform_data);

            #pragma region transforms
            if(context.conditionals.fill_transforms != 0u)
            {
                auto transforms_staging_buffer = context.device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = sizeof(TransformData),
                    .debug_name = "staging_transforms_buffer"
                });

                auto *transform_buffer_ptr = context.device.get_host_address_as<TransformData>(transforms_staging_buffer);
                memcpy(transform_buffer_ptr, &context.buffers.transforms_buffer.cpu_buffer, sizeof(TransformData));

                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = transforms_staging_buffer,
                    .dst_buffer = context.buffers.transforms_buffer.gpu_buffer,
                    .size = sizeof(TransformData),
                });

                cmd_list.destroy_buffer_deferred(transforms_staging_buffer);
                context.conditionals.fill_transforms = static_cast<u32>(false);
            }
            #pragma endregion transforms

            #pragma region scene_data
            if(context.conditionals.fill_scene_geometry != 0u)
            {
                DEBUG_OUT("uploading scene data");
                auto vertices_staging_buffer = context.device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = static_cast<u32>(sizeof(SceneGeometryVertices) * context.buffers.scene_vertices.cpu_buffer.size()),
                    .debug_name = "staging_vertices_buffer"
                });

                auto *vertices_buffer_ptr = context.device.get_host_address_as<SceneGeometryVertices>(vertices_staging_buffer);
                memcpy(vertices_buffer_ptr, context.buffers.scene_vertices.cpu_buffer.data(), 
                    static_cast<u32>(sizeof(SceneGeometryVertices) * context.buffers.scene_vertices.cpu_buffer.size()));

                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = vertices_staging_buffer,
                    .dst_buffer = context.buffers.scene_vertices.gpu_buffer,
                    .size = static_cast<u32>(sizeof(SceneGeometryVertices) * context.buffers.scene_vertices.cpu_buffer.size()),
                });

                auto indices_staging_buffer = context.device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = static_cast<u32>(sizeof(SceneGeometryIndices) * context.buffers.scene_indices.cpu_buffer.size()),
                    .debug_name = "staging_indices_buffer"
                });

                auto *indices_buffer_ptr = context.device.get_host_address_as<SceneGeometryIndices>(indices_staging_buffer);
                memcpy(indices_buffer_ptr, context.buffers.scene_indices.cpu_buffer.data(), 
                    static_cast<u32>(sizeof(SceneGeometryIndices) * context.buffers.scene_indices.cpu_buffer.size()));

                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = indices_staging_buffer,
                    .dst_buffer = context.buffers.scene_indices.gpu_buffer,
                    .size = static_cast<u32>(sizeof(SceneGeometryIndices) * context.buffers.scene_indices.cpu_buffer.size()),
                });

                cmd_list.destroy_buffer_deferred(vertices_staging_buffer);
                cmd_list.destroy_buffer_deferred(indices_staging_buffer);
                context.conditionals.fill_scene_geometry = static_cast<u32>(false);
            }
            #pragma endregion scene_data

        },
        .debug_name = "upload buffer data",
    });
}