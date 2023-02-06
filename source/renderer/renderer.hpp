#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>
#include <daxa/utils/pipeline_manager.hpp>

#include "renderer_context.hpp"
#include "../camera.hpp"
#include "../scene.hpp"
#include "../window.hpp"

#include "tasks/draw_scene_task.hpp"
#include "tasks/draw_imgui_task.hpp"
#include "tasks/fill_buffers_task.hpp"
#include "tasks/draw_debug_ligts.hpp"
#include "tasks/taa_task.hpp"
#include "tasks/init_resolve_image.hpp"
#include "tasks/blit_swapchain_to_resolve.hpp"

struct Renderer
{

    explicit Renderer(const AppWindow & window);
    ~Renderer();

    void resize();
    void draw(Camera & camera);
    void reload_scene_data(const Scene & scene);

    private:
        RendererContext context;
        void create_main_task();
};