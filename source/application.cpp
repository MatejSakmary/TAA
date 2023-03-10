#include "application.hpp"
#include <bit>
#include <imgui_stdlib.h>

#include "utils.hpp"

void Application::mouse_callback(const f64 x, const f64 y)
{
    f32 x_offset;
    f32 y_offset;
    if(!state.first_input)
    {
        x_offset = x - state.last_mouse_pos.x;
        y_offset = y - state.last_mouse_pos.y;
    } else {
        x_offset = 0.0f;
        y_offset = 0.0f;
        state.first_input = false;
    }

    if(state.fly_cam)
    {
        state.last_mouse_pos = {x, y};
        camera.update_front_vector(x_offset, y_offset);
    }
}

void Application::mouse_button_callback(const i32 button, const i32 action, const i32 mods)
{

}

void Application::window_resize_callback(const i32 width, const i32 height)
{
    renderer.resize();
    camera.aspect_ratio = f32(width) / f32(height);
}

void Application::key_callback(const i32 key, const i32 code, const i32 action, const i32 mods)
{
    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        auto update_state = [](i32 action) -> unsigned int
        {
            if(action == GLFW_PRESS) return 1;
            return 0;
        };

        switch (key)
        {
            case GLFW_KEY_W: state.key_table.bits.W = update_state(action); return;
            case GLFW_KEY_A: state.key_table.bits.A = update_state(action); return;
            case GLFW_KEY_S: state.key_table.bits.S = update_state(action); return;
            case GLFW_KEY_D: state.key_table.bits.D = update_state(action); return;
            case GLFW_KEY_Q: state.key_table.bits.Q = update_state(action); return;
            case GLFW_KEY_E: state.key_table.bits.E = update_state(action); return;
            case GLFW_KEY_SPACE: state.key_table.bits.SPACE = update_state(action); return;
            case GLFW_KEY_LEFT_SHIFT: state.key_table.bits.LEFT_SHIFT = update_state(action); return;
            case GLFW_KEY_LEFT_CONTROL: state.key_table.bits.CTRL = update_state(action); return;
            default: break;
        }
    }

    if(key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        state.fly_cam = !state.fly_cam;
        if(state.fly_cam)
        {
            window.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            state.first_input = true;
        } else {
            window.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void Application::ui_update()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoDocking  | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse   |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();


    ImGui::Begin("Info");
    ImGui::Text("Camera position is: ");
    ImGui::SameLine();
    ImGui::Text("%s", glm::to_string(camera.get_camera_position()).c_str());
    if (ImGui::Button("Reload Scene", {100, 20})) { state.file_browser.Open(); }

    ImGui::Checkbox("Jitter camera", &state.current.jitter_camera);

    ImGui::Checkbox("Accumulate", &state.current.accumulate);

    if(!state.current.accumulate) { ImGui::BeginDisabled(); }
    ImGui::Checkbox("Color clamp", &state.current.color_clamp);
    ImGui::Checkbox("Nearest depth", &state.current.nearest_depth);
    ImGui::Checkbox("Reproject velocity", &state.current.reproject_velocity);
    if(!state.current.accumulate) { ImGui::EndDisabled(); }

    if(!state.current.reproject_velocity) {ImGui::BeginDisabled(); }
    ImGui::Checkbox("Reject velocity", &state.current.reject_velocity);
    if(!state.current.reproject_velocity) {ImGui::EndDisabled(); }

    ImGui::Text((std::string("Scene draw time took : ") + std::to_string(renderer.draw_time) + std::string(" ns")).c_str());
    ImGui::Text((std::string("TAA time took : ") + std::to_string(renderer.taa_time) + std::string(" ns")).c_str());

    ImGui::End();

    state.file_browser.Display();

    if(state.file_browser.HasSelected())
    {
        reload_scene(state.file_browser.GetSelected().string());
        state.file_browser.ClearSelected();
    }

    ImGui::Render();
}

Application::Application() : 
    window({1920, 1020},
    WindowVTable {
        .mouse_pos_callback = [this](const f64 x, const f64 y)
            {this->mouse_callback(x, y);},
        .mouse_button_callback = [this](const i32 button, const i32 action, const i32 mods)
            {this->mouse_button_callback(button, action, mods);},
        .key_callback = [this](const i32 key, const i32 code, const i32 action, const i32 mods)
            {this->key_callback(key, code, action, mods);},
        .window_resized_callback = [this](const i32 width, const i32 height)
            {this->window_resize_callback(width, height);},
    }),
    state{ 
        .minimized = 0u,
        .file_browser = ImGui::FileBrowser(ImGuiFileBrowserFlags_NoModal),
    },
    renderer{window},
    camera {{
        .position = {0.0, 0.0, 5.0},
        .front = {0.0, 0.0, -1.0},
        .up = {0.0, 1.0, 0.0}, 
        .aspect_ratio = 1920.0f/1080.0f,
        .fov = glm::radians(30.0f)
    }},
    scene{"resources/suzanne_scene/suzanne.fbx"}
{
    state.file_browser.SetTitle("Select scene file");
    state.file_browser.SetTypeFilters({ ".fbx", ".obj" });
}

void Application::reload_scene(const std::string & path)
{
    scene = Scene(path);
    renderer.reload_scene_data(scene);
}

void Application::update_app_state()
{
    f64 this_frame_time = glfwGetTime();
    state.delta_time =  this_frame_time - state.last_frame_time;
    state.last_frame_time = this_frame_time;

    if(state.key_table.data > 0 && state.fly_cam == true)
    {
        if(state.key_table.bits.W)      { camera.move_camera(state.delta_time, Direction::FORWARD);    }
        if(state.key_table.bits.A)      { camera.move_camera(state.delta_time, Direction::LEFT);       }
        if(state.key_table.bits.S)      { camera.move_camera(state.delta_time, Direction::BACK);       }
        if(state.key_table.bits.D)      { camera.move_camera(state.delta_time, Direction::RIGHT);      }
        if(state.key_table.bits.Q)      { camera.move_camera(state.delta_time, Direction::ROLL_LEFT);  }
        if(state.key_table.bits.E)      { camera.move_camera(state.delta_time, Direction::ROLL_RIGHT); }
        if(state.key_table.bits.CTRL)   { camera.move_camera(state.delta_time, Direction::DOWN);       }
        if(state.key_table.bits.SPACE)  { camera.move_camera(state.delta_time, Direction::UP);         }
    }

    bool changed = false;
    if(state.last_frame.accumulate != state.current.accumulate)
    {
        changed = true;
        renderer.change_shader_define(Define::ACCUMULATE, state.current.accumulate);
        state.current.color_clamp = false;
        state.current.nearest_depth = false;
        state.current.reject_velocity = false;
        state.current.reproject_velocity = false;
    }
    if(state.last_frame.color_clamp != state.current.color_clamp)
    {
        changed = true;
        renderer.change_shader_define(Define::COLOR_CLAMP, state.current.color_clamp);
    }
    if(state.last_frame.jitter_camera != state.current.jitter_camera)
    {
        changed = true;
        renderer.change_shader_define(Define::JITTER, state.current.jitter_camera);
    }
    if(state.last_frame.nearest_depth != state.current.nearest_depth)
    {
        changed = true;
        renderer.change_shader_define(Define::NEAREST_DEPTH, state.current.nearest_depth);
    }
    if(state.last_frame.reproject_velocity != state.current.reproject_velocity)
    {
        changed = true;
        renderer.change_shader_define(Define::REPROJECT_VELOCITY, state.current.reproject_velocity);
        state.current.reject_velocity = false;
    }
    if(state.last_frame.reject_velocity != state.current.reject_velocity)
    {
        changed = true;
        renderer.change_shader_define(Define::REJECT_VELOCITY, state.current.reject_velocity);
    }
    if(changed) { renderer.reload_taa_pipeline(); }
    state.last_frame = state.current;
}

void Application::main_loop()
{
    while (!window.get_window_should_close())
    {
        glfwPollEvents();
        ui_update();
        update_app_state();

        if (state.minimized != 0u) { DEBUG_OUT("[Application::main_loop()] Window minimized "); continue; } 
        renderer.draw(camera);
    }
}
