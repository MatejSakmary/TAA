#pragma once

#include "external/imgui_file_dialog.hpp"
#include "window.hpp"
#include "types.hpp"
#include "scene.hpp"
#include "renderer/renderer.hpp"

struct Application 
{
    struct CheckboxState
    {
        bool jitter_camera = true;
        bool reject_velocity = true;
        bool reproject_velocity = true;
        bool color_clamp = true;
        bool nearest_depth = true;
        bool accumulate = true;
    };

    struct AppState
    {
        union KeyTable
        {
            unsigned int data;
            struct
            {
                unsigned int W : 1 = 0;
                unsigned int A : 1 = 0;
                unsigned int S : 1 = 0;
                unsigned int D : 1 = 0;
                unsigned int Q : 1 = 0;
                unsigned int E : 1 = 0;
                unsigned int CTRL : 1 = 0;
                unsigned int SPACE : 1 = 0;
                unsigned int LEFT_SHIFT : 1 = 0;
            } bits;
        };

        f64 last_frame_time = 0.0;
        f64 delta_time = 0.0;
        b32 minimized = 0u;
        b32 fly_cam = 0u;
        b32 first_input = 1u;
        f32vec2 last_mouse_pos;
        ImGui::FileBrowser file_browser;

        KeyTable key_table;
        CheckboxState last_frame;
        CheckboxState current;
    };

    public:
        Application();
        ~Application() = default;

        void main_loop();

    private:
        AppWindow window;
        AppState state;
        Renderer renderer;
        Camera camera;
        Scene scene;

        void init_window();
        void mouse_callback(const f64 x, const f64 y);
        void mouse_button_callback(const i32 button, const i32 action, const i32 mods);
        void window_resize_callback(const i32 width, const i32 height);
        void key_callback(const i32 key, const i32 code, const i32 action, const i32 mods);
        void reload_scene(const std::string & path);
        void ui_update();
        void update_app_state();
};
