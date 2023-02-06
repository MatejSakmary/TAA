#pragma once

#include <array>

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/math_operators.hpp>

#include "../../types.hpp"
#include "../renderer_context.hpp"
#include "../shared/shared.inl"

inline void task_init_resolve(RendererContext & context)
{
    context.main_task_list.task_list.add_task({
        .used_buffers = {},
        .used_images = {},
        .task = [&](daxa::TaskRuntime const & runtime)
        {
            auto cmd_list = runtime.get_command_list();
            if(context.conditionals.clear_resolve)
            {
                cmd_list.pipeline_barrier_image_transition({
                    .awaited_pipeline_access = daxa::AccessConsts::BOTTOM_OF_PIPE_READ_WRITE,
                    .waiting_pipeline_access = daxa::AccessConsts::TOP_OF_PIPE_READ_WRITE,
                    .before_layout = daxa::ImageLayout::UNDEFINED,
                    .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                    .image_slice = {.image_aspect = daxa::ImageAspectFlagBits::COLOR},
                    .image_id = context.resolve_image
                });
                context.conditionals.clear_resolve = false;
            }
        },
        .debug_name = "clear resolve image task"
    });
}