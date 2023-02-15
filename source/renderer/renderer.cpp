#include "renderer.hpp"

Renderer::Renderer(const AppWindow & window) :
    context {.vulkan_context = daxa::create_context({.enable_validation = true})}
{
    context.device = context.vulkan_context.create_device({.debug_name = "Daxa device"});
    context.swapchain = context.device.create_swapchain({ 
        .native_window = window.get_native_handle(),
        .native_window_platform = daxa::NativeWindowPlatform::XLIB_API,
        .present_mode = daxa::PresentMode::TRIPLE_BUFFER_WAIT_FOR_VBLANK,
        .image_usage = 
            daxa::ImageUsageFlagBits::TRANSFER_DST |
            daxa::ImageUsageFlagBits::TRANSFER_SRC |
            daxa::ImageUsageFlagBits::COLOR_ATTACHMENT |
            daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
        .debug_name = "Swapchain",
    });

    context.pipeline_manager = daxa::PipelineManager({
        .device = context.device,
        .shader_compile_options = {
            .root_paths = {
                DAXA_SHADER_INCLUDE_DIR,
                "source/renderer",
                "source/renderer/shaders",},
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true
        },
        .debug_name = "Pipeline Compiler",
    });

    context.timestamps = context.device.create_timeline_query_pool({
        .query_count = 4,
        .debug_name = "timeline_query",
    });

    context.linear_sampler = context.device.create_sampler({});
    context.nearest_sampler = context.device.create_sampler({
        .magnification_filter = daxa::Filter::NEAREST,
        .minification_filter = daxa::Filter::NEAREST,
        .mipmap_filter = daxa::Filter::NEAREST
    });
    context.offscreen_format = daxa::Format::R16G16B16A16_SFLOAT;
    context.velocity_format = daxa::Format::R16G16_SFLOAT;
    create_resolution_dependent_resources();

    context.pipelines.p_draw_scene = context.pipeline_manager.add_raster_pipeline(get_draw_scene_pipeline(context)).value();
    context.pipelines.p_draw_debug_lights = context.pipeline_manager.add_raster_pipeline(get_draw_debug_lights_pipeline(context)).value();
    context.pipelines.p_taa_pass = context.pipeline_manager.add_compute_pipeline(get_taa_pass_pipeline(context)).value();
    context.pipelines.p_tonemap_pass = context.pipeline_manager.add_raster_pipeline(get_tonemap_pass_pipeline(context)).value();

    context.buffers.transforms_buffer.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = sizeof(TransformData),
        .debug_name = "transform info"
    });

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window.get_glfw_window_handle(), true);
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    context.imgui_renderer = daxa::ImGuiRenderer({
        .device = context.device,
        .format = context.swapchain.get_format(),
    });
    create_main_task();
}

void Renderer::create_resolution_dependent_resources()
{
    auto extent = context.swapchain.get_surface_extent();

    if(context.device.is_id_valid((context.main_task_list.prev_velocity_image)))
    {
        context.main_task_list.task_list.remove_runtime_image(
            context.main_task_list.images.t_prev_velocity_image, context.main_task_list.prev_velocity_image);
        context.device.destroy_image(context.main_task_list.prev_velocity_image);
    }

    if(context.device.is_id_valid((context.main_task_list.velocity_image)))
    {
        context.main_task_list.task_list.remove_runtime_image(
            context.main_task_list.images.t_velocity_image, context.main_task_list.velocity_image);
        context.device.destroy_image(context.main_task_list.velocity_image);
    }

    if(context.device.is_id_valid((context.main_task_list.accumulation_image)))
    {
        context.main_task_list.task_list.remove_runtime_image(
            context.main_task_list.images.t_accumulation_image, context.main_task_list.accumulation_image);
        context.device.destroy_image(context.main_task_list.accumulation_image);
    }

    if(context.device.is_id_valid((context.main_task_list.offscreen_image)))
    {
        context.main_task_list.task_list.remove_runtime_image(
            context.main_task_list.images.t_offscreen_image, context.main_task_list.offscreen_image);
        context.device.destroy_image(context.main_task_list.offscreen_image);
    }

    if(context.device.is_id_valid((context.offscreen_copy_image)))
    {
        context.main_task_list.task_list.remove_runtime_image(
            context.main_task_list.images.t_offscreen_copy_image, context.offscreen_copy_image);
        context.device.destroy_image(context.offscreen_copy_image);
    }

    if(context.device.is_id_valid((context.depth_image)))
    {
        context.main_task_list.task_list.remove_runtime_image(
            context.main_task_list.images.t_depth_image, context.depth_image);
        context.device.destroy_image(context.depth_image);
    }

    context.depth_image = context.device.create_image({
        .format = daxa::Format::D32_SFLOAT,
        .aspect = daxa::ImageAspectFlagBits::DEPTH,
        .size   = {extent.x, extent.y, 1},
        .usage  = 
            daxa::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT |
            daxa::ImageUsageFlagBits::SHADER_READ_ONLY,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .debug_name   = "depth image"
    });

    daxa::ImageUsageFlags attachment_usage = 
        daxa::ImageUsageFlagBits::TRANSFER_DST     |
        daxa::ImageUsageFlagBits::SHADER_READ_ONLY |
        daxa::ImageUsageFlagBits::COLOR_ATTACHMENT |
        daxa::ImageUsageFlagBits::SHADER_READ_WRITE;

    context.velocity_image_1 = context.device.create_image({
        .format = context.velocity_format,
        .aspect = daxa::ImageAspectFlagBits::COLOR,
        .size = {extent.x, extent.y, 1},
        .usage = attachment_usage,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .debug_name = "velocity image 2"
    });

    context.velocity_image_2 = context.device.create_image({
        .format = context.velocity_format,
        .aspect = daxa::ImageAspectFlagBits::COLOR,
        .size = {extent.x, extent.y, 1},
        .usage = attachment_usage,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .debug_name = "velocity image 1"
    });

    context.offscreen_image_1 = context.device.create_image({
        .format = context.offscreen_format,
        .aspect = daxa::ImageAspectFlagBits::COLOR,
        .size = {extent.x, extent.y, 1},
        .usage = attachment_usage,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .debug_name = "offscreen image 1"
    });

    context.offscreen_image_2 = context.device.create_image({
        .format = context.offscreen_format,
        .aspect = daxa::ImageAspectFlagBits::COLOR,
        .size = {extent.x, extent.y, 1},
        .usage = attachment_usage,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .debug_name = "offscreen image 2"
    });

    context.offscreen_copy_image = context.device.create_image({
        .format = context.offscreen_format,
        .aspect = daxa::ImageAspectFlagBits::COLOR,
        .size = {extent.x, extent.y, 1},
        .usage = attachment_usage,
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .debug_name = "offscreen copy"
    });

    context.main_task_list.offscreen_image = context.offscreen_image_1;
    context.main_task_list.accumulation_image = context.offscreen_image_2;

    context.main_task_list.velocity_image = context.velocity_image_1;
    context.main_task_list.prev_velocity_image = context.velocity_image_2;
}

void Renderer::create_main_task()
{
    context.main_task_list.task_list = daxa::TaskList({
        .device = context.device,
        .reorder_tasks = true,
        .use_split_barriers = true,
        .swapchain = context.swapchain,
        .debug_name = "main_tasklist"
    });

    context.main_task_list.images.t_swapchain_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .initial_layout = daxa::ImageLayout::UNDEFINED,
            .swapchain_image = true,
            .debug_name = "t_swapchain_image"
        }
    );

    context.main_task_list.images.t_velocity_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .initial_layout = daxa::ImageLayout::UNDEFINED,
            .swapchain_image = false,
            .debug_name = "t_velocity_image"
        }
    );
    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_velocity_image,
        context.main_task_list.velocity_image);

    context.main_task_list.images.t_prev_velocity_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::COMPUTE_SHADER_READ,
            .initial_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .swapchain_image = false,
            .debug_name = "t_prev_velocity_image"
        }
    );
    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_prev_velocity_image,
        context.main_task_list.prev_velocity_image);

    context.main_task_list.images.t_offscreen_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .initial_layout = daxa::ImageLayout::UNDEFINED,
            .swapchain_image = false,
            .debug_name = "t_offscreen_image"
        }
    );
    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_offscreen_image,
        context.main_task_list.offscreen_image);


    context.main_task_list.images.t_accumulation_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::COMPUTE_SHADER_READ,
            .initial_layout = daxa::ImageLayout::READ_ONLY_OPTIMAL,
            .swapchain_image = false,
            .debug_name = "t_accumulation_image"
        }
    );
    context.main_task_list.task_list.add_runtime_image(
            context.main_task_list.images.t_accumulation_image,
            context.main_task_list.accumulation_image);


    context.main_task_list.images.t_offscreen_copy_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .initial_layout = daxa::ImageLayout::UNDEFINED,
            .swapchain_image = false,
            .debug_name = "t_offscreen_copy_image"
        }
    );

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_offscreen_copy_image,
        context.offscreen_copy_image);

    context.main_task_list.images.t_depth_image = 
        context.main_task_list.task_list.create_task_image(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .initial_layout = daxa::ImageLayout::UNDEFINED,
            .swapchain_image = false,
            .debug_name = "t_depth_image"
        }
    );

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_depth_image,
        context.depth_image);

    #pragma region camera_transforms
    context.main_task_list.buffers.t_transform_data = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_transform_data"
        }
    );
    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_transform_data,
        context.buffers.transforms_buffer.gpu_buffer);
    #pragma endregion camera_transforms

    context.main_task_list.buffers.t_scene_vertices = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_scene_vertices"
        }
    );
    
    context.main_task_list.buffers.t_scene_indices = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_scene_indices"
        }
    );

    context.main_task_list.buffers.t_scene_lights = 
        context.main_task_list.task_list.create_task_buffer(
        {
            .initial_access = daxa::AccessConsts::NONE,
            .debug_name = "t_scene_lights"
        }
    );

    task_fill_buffers(context);
    task_init_accumulation_image(context);
    task_draw_scene(context);
    task_draw_debug_ligts(context);
    task_taa_pass(context);
    task_tonemap_pass(context);
    task_draw_imgui(context);
    context.main_task_list.task_list.submit({});
    context.main_task_list.task_list.present({});
    context.main_task_list.task_list.complete();
}

void Renderer::resize()
{
    context.swapchain.resize();
    create_resolution_dependent_resources();

    context.conditionals.clear_accumulation = true;

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_velocity_image,
        context.main_task_list.velocity_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_prev_velocity_image,
        context.main_task_list.prev_velocity_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_offscreen_image,
        context.main_task_list.offscreen_image);

    context.main_task_list.task_list.add_runtime_image(
            context.main_task_list.images.t_accumulation_image,
            context.main_task_list.accumulation_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_offscreen_copy_image,
        context.offscreen_copy_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_depth_image,
        context.depth_image);
}

void Renderer::swap_offscreen_images()
{
    // VELOCITY
    context.main_task_list.task_list.remove_runtime_image(
        context.main_task_list.images.t_prev_velocity_image,
        context.main_task_list.prev_velocity_image);

    context.main_task_list.task_list.remove_runtime_image(
        context.main_task_list.images.t_velocity_image,
        context.main_task_list.velocity_image);

    std::swap(context.main_task_list.velocity_image, context.main_task_list.prev_velocity_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_prev_velocity_image,
        context.main_task_list.prev_velocity_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_velocity_image,
        context.main_task_list.velocity_image);

    // OFFSCREEN
    context.main_task_list.task_list.remove_runtime_image(
        context.main_task_list.images.t_offscreen_image,
        context.main_task_list.offscreen_image);

    context.main_task_list.task_list.remove_runtime_image(
        context.main_task_list.images.t_accumulation_image,
        context.main_task_list.accumulation_image);

    std::swap(context.main_task_list.offscreen_image, context.main_task_list.accumulation_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_offscreen_image,
        context.main_task_list.offscreen_image);

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_accumulation_image,
        context.main_task_list.accumulation_image);
}

void Renderer::draw(Camera & camera)
{
    auto extent = context.swapchain.get_surface_extent();
    auto m_proj_view = camera.get_view_projection_matrix({
        .near_plane = 0.1f,
        .far_plane = 500.0f,
        .swapchain_extent = {extent.x, extent.y}
    });

    auto m_prev_proj_view = context.buffers.transforms_buffer.cpu_buffer.m_proj_view;
    auto m_inv_proj_view = glm::inverse(m_proj_view);
    auto m_jitter = camera.get_camera_jitter_matrix({extent.x, extent.y});

    context.buffers.transforms_buffer.cpu_buffer = {
        .m_prev_proj_view = m_prev_proj_view,
        .m_inv_proj_view = *reinterpret_cast<daxa::f32mat4x4 *>(&m_inv_proj_view),
        .m_proj_view = *reinterpret_cast<daxa::f32mat4x4 *>(&m_proj_view),
        .m_jitter = *reinterpret_cast<daxa::f32mat4x4 *>(&m_jitter)
    };

    context.conditionals.fill_transforms = true;

    context.main_task_list.task_list.remove_runtime_image(
        context.main_task_list.images.t_swapchain_image,
        context.swapchain_image);

    context.swapchain_image = context.swapchain.acquire_next_image();

    context.main_task_list.task_list.add_runtime_image(
        context.main_task_list.images.t_swapchain_image,
        context.swapchain_image);

    if(!context.device.is_id_valid(context.swapchain_image))
    {
        DEBUG_OUT("[Renderer::draw()] Got empty image from swapchain");
        return;
    }
    context.main_task_list.task_list.execute();

    swap_offscreen_images();

    auto result = context.pipeline_manager.reload_all();
    if(result.is_ok()) {
        if (result.value() == true)
        {
            DEBUG_OUT("[Renderer::draw()] Shaders recompiled successfully");
        }
    } else {
        DEBUG_OUT(result.to_string());
    }

    auto query_results = context.timestamps.get_query_results(0, 4);
    if ((query_results[1] != 0u) && (query_results[3] != 0u))
    {
        draw_time = static_cast<f64>(query_results[2] - query_results[0]) / 1000.0;
    }
    if ((query_results[5] != 0u) && (query_results[7] != 0u))
    {
        taa_time = static_cast<f64>(query_results[6] - query_results[4]) / 1000.0;
    }
}

void Renderer::reload_taa_pipeline()
{
    context.pipeline_manager.remove_compute_pipeline(context.pipelines.p_taa_pass);
    context.conditionals.clear_accumulation = true;
    context.pipelines.p_taa_pass = context.pipeline_manager.add_compute_pipeline(get_taa_pass_pipeline(context)).value();
}

void Renderer::change_shader_define(Define define, bool new_value)
{
    if(define == Define::JITTER)
    {
        context.pipeline_manager.remove_raster_pipeline(context.pipelines.p_draw_scene);
        context.conditionals.jitter_camera = new_value;
        context.pipelines.p_draw_scene = context.pipeline_manager.add_raster_pipeline(get_draw_scene_pipeline(context)).value();
        return;
    }

    switch (define)
    {
        case Define::ACCUMULATE: { context.conditionals.accumulate = new_value; break; }
        case Define::COLOR_CLAMP : { context.conditionals.color_clamp = new_value; break; }
        case Define::NEAREST_DEPTH : { context.conditionals.nearest_depth = new_value; break; }
        case Define::REJECT_VELOCITY: { context.conditionals.velocity_rejection = new_value; break; }
        case Define::REPROJECT_VELOCITY: { context.conditionals.velocity_reproject = new_value; break; }
    }
}

void Renderer::reload_scene_data(const Scene & scene)
{
    auto destroy_buffer_if_valid = [&]<typename T>(
        RendererContext::Buffers::SharedBuffer<T> & buffer,
        const daxa::TaskBufferId task_buffer) -> void
    {
        if(context.device.is_id_valid(buffer.gpu_buffer))
        {
            context.main_task_list.task_list.remove_runtime_buffer(task_buffer, buffer.gpu_buffer);
            context.device.destroy_buffer(buffer.gpu_buffer);
            buffer.cpu_buffer.clear();
        }
    };

    destroy_buffer_if_valid(context.buffers.scene_vertices, context.main_task_list.buffers.t_scene_vertices);
    destroy_buffer_if_valid(context.buffers.scene_indices, context.main_task_list.buffers.t_scene_indices);
    destroy_buffer_if_valid(context.buffers.scene_lights, context.main_task_list.buffers.t_scene_lights);

    context.render_info.objects.clear();
    size_t scene_vertex_cnt = 0;
    size_t scene_index_cnt = 0;

    for(const auto & scene_light : scene.scene_lights)
    {
        f32vec4 light_position = scene_light.transform * scene_light.position;
        context.buffers.scene_lights.cpu_buffer.push_back(SceneLights{
            .position = daxa_vec4_from_glm(light_position)
        });
    }

    // pack scene vertices and scene indices into their separate GPU buffers
    for(const auto & scene_object : scene.scene_objects)
    {
        context.render_info.objects.push_back({
            .model_transform = scene_object.transform,
        });
        auto & object = context.render_info.objects.back();

        for(auto scene_runtime_mesh : scene_object.meshes)
        {
            object.meshes.push_back({
                .index_buffer_offset = static_cast<u32>(scene_index_cnt),
                .index_offset = static_cast<u32>(scene_vertex_cnt),
                .index_count = static_cast<u32>(scene_runtime_mesh.indices.size()),
            });
            auto & mesh = object.meshes.back();

            context.buffers.scene_vertices.cpu_buffer.resize(scene_vertex_cnt + scene_runtime_mesh.vertices.size());
            memcpy(context.buffers.scene_vertices.cpu_buffer.data() + scene_vertex_cnt,
                   scene_runtime_mesh.vertices.data(),
                   sizeof(SceneGeometryVertices) * scene_runtime_mesh.vertices.size());
            scene_vertex_cnt += scene_runtime_mesh.vertices.size();

            context.buffers.scene_indices.cpu_buffer.resize(scene_index_cnt + scene_runtime_mesh.indices.size());
            memcpy(context.buffers.scene_indices.cpu_buffer.data() + scene_index_cnt,
                   scene_runtime_mesh.indices.data(),
                   sizeof(u32) * scene_runtime_mesh.indices.size());
            scene_index_cnt += scene_runtime_mesh.indices.size();
        }
    }

    context.buffers.scene_vertices.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = static_cast<u32>(scene_vertex_cnt * sizeof(SceneGeometryVertices)),
        .debug_name = "scene_geometry_vertices"
    });

    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_scene_vertices,
        context.buffers.scene_vertices.gpu_buffer);

    context.buffers.scene_indices.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = static_cast<u32>(scene_index_cnt * sizeof(SceneGeometryIndices)),
        .debug_name = "scene_geometry_indices"
    });

    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_scene_indices,
        context.buffers.scene_indices.gpu_buffer);

    context.buffers.scene_lights.gpu_buffer = context.device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::DEDICATED_MEMORY,
        .size = static_cast<u32>(context.buffers.scene_lights.cpu_buffer.size() * sizeof(SceneLights))
    });

    context.main_task_list.task_list.add_runtime_buffer(
        context.main_task_list.buffers.t_scene_lights,
        context.buffers.scene_lights.gpu_buffer);

    context.conditionals.fill_scene_geometry = static_cast<u32>(true);
    DEBUG_OUT("[Renderer::reload_scene_data()] scene reload successfull");
}

Renderer::~Renderer()
{
    context.device.wait_idle();
    ImGui_ImplGlfw_Shutdown();
    context.device.destroy_image(context.depth_image);
    context.device.destroy_image(context.offscreen_image_1);
    context.device.destroy_image(context.offscreen_image_2);
    context.device.destroy_image(context.offscreen_copy_image);
    context.device.destroy_image(context.velocity_image_1);
    context.device.destroy_image(context.velocity_image_2);
    context.device.destroy_buffer(context.buffers.transforms_buffer.gpu_buffer);
    context.device.destroy_sampler(context.linear_sampler);
    context.device.destroy_sampler(context.nearest_sampler);
    if(context.device.is_id_valid(context.buffers.scene_vertices.gpu_buffer))
    {
        context.device.destroy_buffer(context.buffers.scene_vertices.gpu_buffer);
    }
    if(context.device.is_id_valid(context.buffers.scene_lights.gpu_buffer))
    {
        context.device.destroy_buffer(context.buffers.scene_lights.gpu_buffer);
    }
    if(context.device.is_id_valid(context.buffers.scene_indices.gpu_buffer))
    {
        context.device.destroy_buffer(context.buffers.scene_indices.gpu_buffer);
    }
    context.device.collect_garbage();
}