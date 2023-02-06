#include "camera.hpp"

Camera::Camera(const CameraInfo & info) : 
    position{info.position}, front{info.front}, up{info.up}, aspect_ratio{info.aspect_ratio}, fov{info.fov},
    speed{10.0f}, pitch{0.0f}, yaw{-90.0f}, sensitivity{0.08f}, 
    roll_sensitivity{20.0f}, roll{0.0f}, jitter_idx{0u}
{
}

void Camera::move_camera(f32 delta_time, Direction direction)
{
    switch (direction)
    {
    case Direction::FORWARD:
        position += front * speed * delta_time;
        break;
    case Direction::BACK:
        position -= front * speed * delta_time;
        break;
    case Direction::LEFT:
        position -= glm::normalize(glm::cross(front, up)) * speed * delta_time;
        break;
    case Direction::RIGHT:
        position += glm::normalize(glm::cross(front, up)) * speed * delta_time;
        break;
    case Direction::UP:
        position += glm::normalize(glm::cross(glm::cross(front,up), front)) * speed * delta_time;
        break;
    case Direction::DOWN:
        position -= glm::normalize(glm::cross(glm::cross(front,up), front)) * speed * delta_time;
        break;
    case Direction::ROLL_LEFT:
        up = glm::rotate(up, static_cast<f32>(glm::radians(-roll_sensitivity * delta_time)), front);
        break;
    case Direction::ROLL_RIGHT:
        up = glm::rotate(up, static_cast<f32>(glm::radians(roll_sensitivity * delta_time)), front);
        break;
    
    default:
        DEBUG_OUT("[Camera::move_camera()] Unkown enum value");
        break;
    }
}

void Camera::update_front_vector(f32 x_offset, f32 y_offset)
{

    f32vec3 front_ = glm::rotate(front, glm::radians(-sensitivity * x_offset), up);
    front_ = glm::rotate(front_, glm::radians(-sensitivity * y_offset), glm::cross(front,up));

    pitch = glm::degrees(glm::angle(front_, up));

    const f32 MAX_PITCH_ANGLE = 179.0f;
    const f32 MIN_PITCH_ANGLE = 1.0f;
    if (pitch < MIN_PITCH_ANGLE || pitch > MAX_PITCH_ANGLE ) 
    {
        return;
    }

    front = front_;
}

// source - http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
auto Camera::get_camera_jitter_matrix(const f32vec2 swapchain_extent) -> f32mat4x4
{
    f32vec2 jitter_scale = f32vec2(1.0f/f32(swapchain_extent.x), 1.0f/f32(swapchain_extent.y));
    f32 g = 1.32471795724474602596f;
    f32 a1 = 1.0f / g;
    f32 a2 = 1.0f / (g * g);

    f32 tmp = glm::mod(0.5f + a1 * (jitter_idx + 1.0f), 1.0f) - 0.5f;
    f32vec2 jitter = f32vec2(
        glm::mod(0.5f + a1 * (jitter_idx + 1.0f), 1.0f) - 0.5f,
        glm::mod(0.5f + a2 * (jitter_idx + 1.0f), 1.0f) - 0.5f
    );
    jitter = jitter * jitter_scale;
    jitter_idx = (jitter_idx + 1) % 32; 
    f32mat4x4 jitter_mat(1.0);
    jitter_mat[3][0] = jitter.x;
    jitter_mat[3][1] = jitter.y;

    return jitter_mat;
}

auto Camera::get_view_projection_matrix(const GetViewProjectionInfo & info) -> f32mat4x4
{
    f32mat4x4 m_proj = glm::perspective(fov, aspect_ratio, info.near_plane, info.far_plane);
    /* GLM is using OpenGL standard where Y coordinate of the clip coordinates is inverted */
    m_proj[1][1] *= -1;

    auto m_view = glm::lookAt(position, position + front, up);
    f32mat4x4 m_proj_view = m_proj * m_view;
    return m_proj_view;
}

auto Camera::get_camera_position() const -> f32vec3
{
    return position;
}