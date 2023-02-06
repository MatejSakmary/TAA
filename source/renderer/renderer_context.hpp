#pragma once

#include <vector>
#include <daxa/daxa.hpp>
#include <daxa/utils/task_list.hpp>
#include <daxa/utils/imgui.hpp>

#include "../types.hpp"
#include "../scene.hpp"
#include "../external/imgui_file_dialog.hpp"

#include "shared/shared.inl"

struct RendererContext
{
    struct Buffers
    {
        template<typename T>
        struct SharedBuffer
        {
            T cpu_buffer;
            daxa::BufferId gpu_buffer;
        };

        SharedBuffer<TransformData> transforms_buffer;
        SharedBuffer<std::vector<SceneGeometryVertices>> scene_vertices;
        SharedBuffer<std::vector<SceneGeometryIndices>> scene_indices;
        SharedBuffer<std::vector<SceneLights>> scene_lights;
    };

    struct MainTaskList
    {
        struct TaskListImages
        {
            daxa::TaskImageId t_swapchain_image;
            daxa::TaskImageId t_backbuffer_image;
            daxa::TaskImageId t_resolve_image;
            daxa::TaskImageId t_depth_image;
        };

        struct TaskListBuffers
        {
            daxa::TaskBufferId t_transform_data;

            daxa::TaskBufferId t_scene_vertices;
            daxa::TaskBufferId t_scene_indices;
            daxa::TaskBufferId t_scene_lights;
        };

        daxa::TaskList task_list;
        TaskListImages images;
        TaskListBuffers buffers;
    };

    struct Pipelines
    {
        std::shared_ptr<daxa::RasterPipeline> p_draw_scene;
        std::shared_ptr<daxa::RasterPipeline> p_draw_debug_lights;
        std::shared_ptr<daxa::RasterPipeline> p_taa_pass;
    };

    struct Conditionals
    {
        bool fill_transforms = true;
        bool fill_scene_geometry = false;
        bool clear_resolve = true;
    };

    // TODO(msakmary) perhaps reconsider moving this to Scene?
    struct SceneRenderInfo
    {
        struct RenderMeshInfo
        {
            u32 index_buffer_offset;
            u32 index_offset;
            u32 index_count;
        };
        struct RenderObjectInfo
        {
            f32mat4x4 model_transform;
            std::vector<RenderMeshInfo> meshes;
        };

        std::vector<RenderObjectInfo> objects;
    };

    daxa::Context vulkan_context;
    daxa::Device device;
    daxa::Swapchain swapchain;
    daxa::PipelineManager pipeline_manager;

    daxa::ImageId swapchain_image;
    daxa::ImageId backbuffer_image;
    daxa::ImageId resolve_image;
    daxa::ImageId depth_image;
    daxa::SamplerId nearest_sampler;

    daxa::ImGuiRenderer imgui_renderer;

    Buffers buffers;
    MainTaskList main_task_list;
    Pipelines pipelines;

    Conditionals conditionals;
    SceneRenderInfo render_info;
};