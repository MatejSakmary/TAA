#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/shared.inl"

inline void task_blit_swapchain_to_resolve(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers = {},
        .used_images = 
        {
            {
                context.main_task_list.images.t_resolve_image,
                daxa::TaskImageAccess::TRANSFER_WRITE,
            },
            {
                context.main_task_list.images.t_swapchain_image,
                daxa::TaskImageAccess::TRANSFER_READ,
            }
        },
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            auto swapchain_image = runtime.get_images(context.main_task_list.images.t_swapchain_image);
            auto resolve_image = runtime.get_images(context.main_task_list.images.t_resolve_image);

            auto dimensions = context.swapchain.get_surface_extent();
            cmd_list.blit_image_to_image({
                .src_image = swapchain_image[0],
                .src_image_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL,
                .dst_image = resolve_image[0],
                .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                .src_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR},
                .src_offsets = {{{0,0,0}, {static_cast<i32>(dimensions.x), static_cast<i32>(dimensions.y), 1}}},
                .dst_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR},
                .dst_offsets = {{{0,0,0}, {static_cast<i32>(dimensions.x), static_cast<i32>(dimensions.y), 1}}}
            });
        },
        .debug_name = "blit swapchain image to resolve image"
    });
}