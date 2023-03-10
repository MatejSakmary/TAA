#pragma once

#include "types.hpp"
#include "utils.hpp"

struct CameraInfo
{
    const f32vec3 position;
    const f32vec3 front;
    const f32vec3 up;
    const f32 aspect_ratio;
    const f32 fov;
};

struct GetViewProjectionInfo
{
    f32 near_plane;
    f32 far_plane;
    const f32vec2 swapchain_extent;
};

struct Camera
{
    f32 aspect_ratio;
    f32 fov;

    explicit Camera(const CameraInfo & info);

    void move_camera(f32 delta_time, Direction direction);
    void update_front_vector(f32 x_offset, f32 y_offset);
    [[nodiscard]] auto get_camera_position() const -> f32vec3;
    [[nodiscard]] auto get_view_projection_matrix(const GetViewProjectionInfo & info) -> f32mat4x4;
    [[nodiscard]] auto get_camera_jitter_matrix(const f32vec2 swapchain_extent) -> f32mat4x4;

    private:

        f32vec3 position;
        f32vec3 front;
        f32vec3 up;
        f32 pitch;
        f32 yaw;
        f32 roll;
        f32 speed;
        f32 sensitivity;
        f32 roll_sensitivity;
        u32 jitter_idx;
};