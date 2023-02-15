#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>
#include <daxa/utils/pipeline_manager.hpp>

#include "renderer_context.hpp"
#include "../camera.hpp"
#include "../scene.hpp"
#include "../window.hpp"

#include "tasks/fill_buffers_task.hpp"
#include "tasks/init_accumulation_image.hpp"
#include "tasks/draw_scene_task.hpp"
#include "tasks/draw_debug_ligts.hpp"
#include "tasks/draw_imgui_task.hpp"
#include "tasks/taa_task.hpp"
#include "tasks/tonemap_task.hpp"

enum Define
{
    JITTER,
    COLOR_CLAMP,
    REJECT_VELOCITY,
    REPROJECT_VELOCITY,
    NEAREST_DEPTH,
    ACCUMULATE
};

struct Renderer
{
    explicit Renderer(const AppWindow & window);
    ~Renderer();
    f64 draw_time;
    f64 taa_time;

    void resize();
    void draw(Camera & camera);
    void reload_scene_data(const Scene & scene);
    void change_shader_define(Define define, bool new_value);
    void reload_taa_pipeline();

    private:
        RendererContext context;

        void create_main_task();
        void create_resolution_dependent_resources();
        void swap_offscreen_images();
};