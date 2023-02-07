#pragma once

#include <array>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/shared.inl"

inline void task_init_accumulation_image(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers = {},
        .used_images = {},
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            if(context.conditionals.clear_accumulation)
            {
                auto accumulation_image = runtime.get_images(context.main_task_list.images.t_accumulation_image);
                auto prev_velocity_image = runtime.get_images(context.main_task_list.images.t_prev_velocity_image);
                cmd_list.pipeline_barrier_image_transition({
                    .awaited_pipeline_access = daxa::AccessConsts::BOTTOM_OF_PIPE_READ_WRITE,
                    .waiting_pipeline_access = daxa::AccessConsts::TOP_OF_PIPE_READ_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR},
                    .image_id = accumulation_image[0]
                });
                cmd_list.pipeline_barrier_image_transition({
                    .awaited_pipeline_access = daxa::AccessConsts::BOTTOM_OF_PIPE_READ_WRITE,
                    .waiting_pipeline_access = daxa::AccessConsts::TOP_OF_PIPE_READ_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR},
                    .image_id = prev_velocity_image[0]
                });
            }
        },
        .debug_name = "clear accumulation image task"
    });
}